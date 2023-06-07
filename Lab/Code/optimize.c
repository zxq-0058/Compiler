#include "optimize.h"

#include "basicblock.h"
#include "constprop.h"
#include "dag.h"
#include "deadcode.h"
#include "intercode.h"
#include "logger.h"

extern void print_intercodes(InterCodes head_, FILE *fp);

/// @brief 打印文件内容(调试用)
/// @param file 文件指针
void print_file_content(FILE *file) {
    if (file == NULL) {
        printf("无效的文件指针\n");
        return;
    }
    fseek(file, 0, SEEK_SET);  // 将文件指针移动到文件开头
    char buffer[256];          // 用于存储每行的内容
    // 读取并打印文件内容
    while (fgets(buffer, sizeof(buffer), file)) {
        printf("%s", buffer);
    }
}

/// @brief 给定字符串，创建一个操作数
/// @param buffer #1, t1, v1, label1, func
/// @return Operand
Operand create_operand(char *buffer) {
    Operand op = (Operand)malloc(sizeof(struct Operand_));
    if (buffer[0] == '#') {
        op->kind = OP_CONSTANT;
        op->u.value = atoi(buffer + 1);
    } else if (sscanf(buffer, "t%d", &op->u.tmp.tmp_no) == 1) {
        op->kind = OP_TEMP;
        op->u.tmp.tmp_addr = 0;
    } else if (sscanf(buffer, "v%d", &op->u.var_no) == 1) {
        op->kind = OP_VARIABLE;
    } else if (sscanf(buffer, "label%d", &op->u.var_no) == 1) {
        op->kind = OP_LABEL;
    } else {
        op->kind = OP_FUNCTION;
        char *copy = (char *)malloc(sizeof(char) * strlen(buffer));
        strcpy(copy, buffer);
        op->u.func_name = copy;
    }
    return op;
}

/// @brief 给定字符串，创建一个中间代码
/// @param in 一行中间指令
InterCode create_intercode(char *in) {
    char buffer[256];   // 用于存储每行的内容，可能因为strtok而被破坏
    char buffer_[256];  // 用于存储每行的内容，不会被破坏
    strcpy(buffer, in);
    strcpy(buffer_, in);
    char tmp_str1[16], tmp_str2[16], tmp_str3[16];  // 用于存储临时字符串

    char *token = strtok(buffer, " ");
    if (token == NULL) {
        Panic("should not reach here!");
        return NULL;
    }

    InterCode code = (InterCode)malloc(sizeof(struct InterCode_));
    if (strcmp(token, "LABEL") == 0) {  // LABEL x
        code->kind = IR_LABEL;
        code->u.one.op = create_operand(strtok(NULL, " "));
    } else if (strcmp(token, "FUNCTION") == 0) {  // FUNCTION f
        code->kind = IR_FUNCTION;
        code->u.one.op = create_operand(strtok(NULL, " "));
    } else if (strcmp(token, "GOTO") == 0) {  // GOTO x
        code->kind = IR_GOTO;
        code->u.one.op = create_operand(strtok(NULL, " "));
    } else if (strcmp(token, "RETURN") == 0) {  // RETURN x
        code->kind = IR_RETURN;
        code->u.one.op = create_operand(strtok(NULL, " "));
    } else if (strcmp(token, "READ") == 0) {  // READ x
        code->kind = IR_READ;
        code->u.one.op = create_operand(strtok(NULL, " "));
    } else if (strcmp(token, "WRITE") == 0) {  // WRITE x
        code->kind = IR_WRITE;
        code->u.one.op = create_operand(strtok(NULL, " "));
    } else if (strcmp(token, "ARG") == 0) {  // ARG x
        code->kind = IR_ARG;
        code->u.one.op = create_operand(strtok(NULL, " "));
    } else if (strcmp(token, "PARAM") == 0) {  // PARAM x
        code->kind = IR_PARAM;
        code->u.one.op = create_operand(strtok(NULL, " "));
    } else if (strcmp(token, "DEC") == 0) {  // DEC x [size]
        code->kind = IR_DEC;
        code->u.dec.op = create_operand(strtok(NULL, " "));
        code->u.dec.size = atoi(strtok(NULL, " "));
    } else if (strcmp(token, "IF") == 0) {  // IF x [relop] y GOTO z
        code->kind = IR_IFGOTO;
        code->u.ifgoto.x = create_operand(strtok(NULL, " "));
        char *relop = (char *)malloc(sizeof(char) * 4);
        strcpy(relop, strtok(NULL, " "));
        code->u.ifgoto.relop = relop;
        code->u.ifgoto.y = create_operand(strtok(NULL, " "));
        strtok(NULL, " ");  // skip "GOTO"
        code->u.ifgoto.z = create_operand(strtok(NULL, " "));
    } else if (sscanf(buffer_, "%s := CALL %s", tmp_str1, tmp_str2) == 2) {
        code->kind = IR_CALL;
        code->u.two.left = create_operand(tmp_str1);
        code->u.two.right = create_operand(tmp_str2);
    } else if (sscanf(buffer_, "%s := &%s + %s", tmp_str1, tmp_str2, tmp_str3) == 3) {  // x := &y + z
        code->kind = IR_ADDRADD;
        code->u.binop.result = create_operand(tmp_str1);
        code->u.binop.op1 = create_operand(tmp_str2);
        code->u.binop.op2 = create_operand(tmp_str3);
    } else if (sscanf(buffer_, "%s := %s + %s", tmp_str1, tmp_str2, tmp_str3) == 3) {  // x := y + z
        code->kind = IR_ADD;
        code->u.binop.result = create_operand(tmp_str1);
        code->u.binop.op1 = create_operand(tmp_str2);
        code->u.binop.op2 = create_operand(tmp_str3);
    } else if (sscanf(buffer_, "%s := %s - %s", tmp_str1, tmp_str2, tmp_str3) == 3) {  // x := y - z
        code->kind = IR_SUB;
        code->u.binop.result = create_operand(tmp_str1);
        code->u.binop.op1 = create_operand(tmp_str2);
        code->u.binop.op2 = create_operand(tmp_str3);
    } else if (sscanf(buffer_, "%s := %s * %s", tmp_str1, tmp_str2, tmp_str3) == 3) {  // x := y * z
        code->kind = IR_MUL;
        code->u.binop.result = create_operand(tmp_str1);
        code->u.binop.op1 = create_operand(tmp_str2);
        code->u.binop.op2 = create_operand(tmp_str3);
    } else if (sscanf(buffer_, "%s := %s / %s", tmp_str1, tmp_str2, tmp_str3) == 3) {  // x := y / z
        code->kind = IR_DIV;
        code->u.binop.result = create_operand(tmp_str1);
        code->u.binop.op1 = create_operand(tmp_str2);
        code->u.binop.op2 = create_operand(tmp_str3);
    } else if (sscanf(buffer_, "%s := &%s", tmp_str1, tmp_str2) == 2) {  // x := &y
        code->kind = IR_ASSIGN;
        code->u.assign.ass_type = ASS_GETADDR;
        code->u.assign.left = create_operand(tmp_str1);
        code->u.assign.right = create_operand(tmp_str2);
    } else if (sscanf(buffer_, "%s := *%s", tmp_str1, tmp_str2) == 2) {  // x := *y
        code->kind = IR_ASSIGN;
        code->u.assign.ass_type = ASS_GETVAL;
        code->u.assign.left = create_operand(tmp_str1);
        code->u.assign.right = create_operand(tmp_str2);
    } else if (sscanf(buffer_, "*%s := %s", tmp_str1, tmp_str2) == 2) {  // *x := y
        code->kind = IR_ASSIGN;
        code->u.assign.ass_type = ASS_SETVAL;
        code->u.assign.left = create_operand(tmp_str1);
        code->u.assign.right = create_operand(tmp_str2);
    } else if (sscanf(buffer_, "%s := %s", tmp_str1, tmp_str2) == 2) {  // x := y
        code->kind = IR_ASSIGN;
        code->u.assign.ass_type = ASS_NORMAL;
        code->u.assign.left = create_operand(tmp_str1);
        code->u.assign.right = create_operand(tmp_str2);
    } else {
        Panic("Not implemented yet: %s", buffer_);
    }
    return code;
}

/// @brief 创建中间代码节点
InterCodes create_intercodes_node(InterCode code) {
    InterCodes node = (InterCodes)malloc(sizeof(struct InterCodes_));
    node->code = code;
    node->prev = NULL;
    node->next = NULL;
    return node;
}

/// @brief 输入一个文件名，返回一个InterCodes链表
/// @return InterCodes
InterCodes parse_intercodes(FILE *fp) {
    InterCodes head = NULL;
    InterCodes tail = NULL;
    char buffer[256];

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // 移除行尾的换行符
        buffer[strcspn(buffer, "\n")] = '\0';
        InterCode code = create_intercode(buffer);
        if (code != NULL) {
            InterCodes node = create_intercodes_node(code);
            if (head == NULL) {
                head = node;
                tail = node;
            } else {
                tail->next = node;
                node->prev = tail;
                tail = node;
            }
        }
    }

    return head;
}

/// @brief 优化中间代码
/// @param in 输入文件
/// @param out 输出文件
void optimize_intercodes(FILE *in, FILE *out) {
    // 检查文件指针非空
    Debug("\nOptimizing intercodes...\n");
    if (in == NULL || out == NULL) {
        Panic("File pointer is NULL");
    }
    fseek(in, 0, SEEK_SET);  // 将文件指针移动到文件开头(important)
    InterCodes codes = parse_intercodes(in);
    BasicBlock blocks = NULL;
    int blockCount = 0;
    split_into_basicblocks(codes, &blocks, &blockCount);

#ifdef DEBUG_ON
    Debug("After split into basic blocks:\n");
    print_basicblocks(blocks, blockCount);  // for debugging
#endif

    // 局部优化
    BasicBlock block = blocks;
    while (block != NULL) {
        const_propagate(block);
        common_exp_optimize(block);
        block = block->next;
    }

    create_flowgraphs(blocks);

#ifdef GLOBAL_OPTIMIZE_ON
    global_const_propagation();
    compute_dfn();
    compute_dominators();
    compute_backedges();
    compute_loops();

    loop_const_optimize();

#endif
    flw_graphs_export_code(out);

#ifdef DEBUG_ON
    flw_graphs_export_code(stdout);
#endif
}

// 69473269
// 69676043