#include "objcode.h"

FILE *spm_code_file = NULL;
extern InterCodes head;
extern char *print_operand(Operand op);

extern int var_count;
extern int tmp_count;

reg_t registers[NUM_REGISTERS];

#define inf 0x3f3f3f3f
#define RELEASE_REG(reg) (reg->is_free = 1)
// v%d和t%d在栈帧中的位置数组
int *offsets;

/// @brief 为操作数op分配栈帧中的位置（如果已经有offset那么返回，只在init_stack_record使用）
/// @param op  操作数
/// @param offsets 可能对原先的offsets进行修改（相对于fp的偏移）
void alloc_offset(Operand op, int *current_offset) {
    // 由于OP_ADDRESS仅仅用于函数形式参数为数组或者结构体时，所以这里不需要考虑
    Panic_ON(op->kind != OP_VARIABLE && op->kind != OP_TEMP, "Invalid Operand");
    if (op->kind == OP_VARIABLE) {
        if (offsets[op->u.var_no] == -inf) {
            offsets[op->u.var_no] = *current_offset;
            *current_offset += 4;
        }
        return;
    } else {
        if (offsets[op->u.tmp.tmp_no + var_count] == -inf) {
            offsets[op->u.tmp.tmp_no + var_count] = *current_offset;
            *current_offset += 4;
        }
        return;
    }
}

/// @brief 为每一个函数中的变量(v和d)初始化在栈帧中的位置(初始化一个函数，只在FUNCTION指令时调用)
int init_stack_record(InterCodes func) {
    Panic_ON(!func || func->code->kind != OP_FUNCTION, "Invalid Operand");
    InterCodes p = func->next;
    int fp_offset = 0;
    while (p && p->code->kind != IR_FUNCTION) {
        switch (p->code->kind) {
            case IR_DEC: {
                Operand op = p->code->u.dec.op;
                offsets[op->u.var_no] = fp_offset + p->code->u.dec.size - 4;
                fp_offset += p->code->u.dec.size;
                break;
            }
            case IR_PARAM:
                Panic("Not implemented yet");
                break;
            case IR_READ:
                alloc_offset(p->code->u.one.op, &fp_offset);
                break;
            case IR_WRITE:
                alloc_offset(p->code->u.one.op, &fp_offset);
                break;
            case IR_ASSIGN: {
                alloc_offset(p->code->u.assign.left, &fp_offset);
                alloc_offset(p->code->u.assign.right, &fp_offset);
                break;
            }
            case IR_ADD:
            case IR_ADDRADD:
            case IR_SUB:
            case IR_MUL:
            case IR_DIV: {
                alloc_offset(p->code->u.binop.result, &fp_offset);
                alloc_offset(p->code->u.binop.op1, &fp_offset);
                alloc_offset(p->code->u.binop.op2, &fp_offset);
                break;
            }
            case IR_GOTO:
                Panic("Not implemented yet");
                break;
            case IR_IFGOTO:
                Panic("Not implemented yet");
                break;
            case IR_RETURN:
                // 什么都不做
                break;
            case IR_ARG:
                Panic("Not implemented yet");
                break;
            case IR_CALL:
                Panic("Not implemented yet");
                break;
            default:
                Panic("Invalid InterCode");
                break;
        }
        p = p->next;
    }
    printf("Function %s has %d bytes of local variables\n", func->code->u.one.op->u.func_name, fp_offset);
    return fp_offset;
}

/// @brief 获得一个寄存器
reg_t *get_reg() {
    for (int i = 0; i < NUM_REGISTERS; i++) {
        if (registers[i].is_free) {
            registers[i].is_free = 0;  // 标记为不可用
            return &registers[i];
        }
    }
    // 朴素寄存器实现中，这里不应该被执行到
    Panic("Should not reach here in naive register allocation.");
}

/// @brief 将一个操作数加载到一个寄存器中
void load2reg(Operand op, reg_t *reg) {
    if (op->kind == OP_CONSTANT) {
        fprintf(spm_code_file, "  li %s, %d\n", reg->name, op->u.value);
    } else {
        int off = get_offset(op);
        fprintf(spm_code_file, "  lw %s, %d($fp)\n", reg->name, off);
    }
}

/// @brief 将一个寄存器中的值存入一个操作数的地址
void store2reg(Operand op, reg_t *reg) {
    int off = get_offset(op);
    fprintf(spm_code_file, "  sw %s, %d($fp)\n", reg->name, off);
}

/// @brief 获得一个变量的栈帧偏移（在此之前已经alloc_offset）
int get_offset(Operand op) {
    int off = -inf;
    if (op->kind == OP_VARIABLE) {
        off = offsets[op->u.var_no];
    } else {
        off = offsets[op->u.tmp.tmp_no + var_count];
    }
    Panic_ON(off == -inf, "Operand should have been allocated a stack offset.");
    return off;
}

/// @brief 实验中的mips的寄存器(for debugging)
const char *reg_names[] = {
    "$zero",                       // 常数0
    "$at",                         // 保留给汇编器
    "$v0",   "$v1",                // 函数返回值或者表达式求值(在实验中只会用到v0,v1另作他用)
    "$a0",   "$a1", "$a2", "$a3",  // 函数参数（跨函数不保留）
    "$t0",   "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",  // 临时变量（跨函数不保留）
    "$s0",   "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",  // 保存变量（跨函数不保留）
    "$t8",   "$t9",  // 临时变量， 函数调用者负责保存（跨函数不保留）
    "$k0",   "$k1",  // 保留给操作系统（系统调用）
    "$gp",           // 全局指针
    "$sp",           // 栈指针
    "$fp",           // 帧指针
    "$ra"            // 返回地址
};

/// @brief 初始化目标代码（实际上就是将讲义上的固定代码写入spm_code_file）
void obj_init() {
    const char *str =
        ".data\n"
        "_prompt: .asciiz \"Enter an integer:\"\n"
        "_ret: .asciiz \"\\n\"\n"
        ".globl main\n"
        ".text\n"
        "read:\n"
        "  li $v0, 4\n"
        "  la $a0, _prompt\n"
        "  syscall\n"
        "  li $v0, 5\n"
        "  syscall\n"
        "  jr $ra\n"
        "write:\n"
        "  li $v0, 1\n"
        "  syscall\n"
        "  li $v0, 4\n"
        "  la $a0, _ret\n"
        "  syscall\n"
        "  move $v0, $0\n"
        "  jr $ra\n";
    fprintf(spm_code_file, "%s", str);

    // 初始化偏移量数组
    offsets = (int *)malloc((var_count + tmp_count) * sizeof(int));
    for (int i = 0; i < var_count + tmp_count; i++) {
        offsets[i] = -inf;
    }

    // 初始化临时寄存器数组
    for (int i = 0; i < NUM_REGISTERS; i++) {
        sprintf(registers[i].name, "$t%d", i);
        registers[i].is_free = 1;  // 初始时所有寄存器都可用
    }
}

/// @brief 将中间代码转换为目标代码
void ir2obj(FILE *fp) {
    spm_code_file = fp;
    obj_init();
    InterCodes p = head;
    while (p != NULL) {
        switch (p->code->kind) {
            case IR_LABEL:
                fprintf(spm_code_file, "%s:\n", print_operand(p->code->u.one.op));
                break;
            case IR_FUNCTION: {
                fun_obj(p);
                break;
            }
            case IR_ASSIGN: {
                assign_obj(p->code);
                break;
            }
            case IR_ADD:
            case IR_ADDRADD:
            case IR_SUB:
            case IR_MUL:
            case IR_DIV: {
                binary_obj(p->code);
                break;
            }
            case IR_GOTO:
                fprintf(spm_code_file, "j %s\n", print_operand(p->code->u.one.op));
                break;
            case IR_IFGOTO:
                Panic("Not implemented yet");
                break;
            case IR_RETURN: {
                return_obj(p->code->u.one.op);
                break;
            }
            case IR_DEC:  // 内存申请已经在init_stack_record中完成
                break;
            case IR_ARG:
                Panic("Not implemented yet");
                break;
            case IR_CALL:
                Panic("Not implemented yet");
                break;
            case IR_PARAM:
                Panic("Not implemented yet");
                break;
            case IR_READ:
                read_obj(p->code->u.one.op);
                break;
            case IR_WRITE:
                write_obj(p->code->u.one.op);
                break;
            default:
                Panic("Invalid InterCode");
                break;
        }
        p = p->next;
    }
}

/// @brief 中间代码为函数声明
static void inline fun_obj(InterCodes codes) {
    fprintf(spm_code_file, "%s:\n", codes->code->u.one.op->u.func_name);
    const char *str =
        "  addi $sp, $sp, -4\n"
        "  sw $fp 0($sp)\n"
        "  move $fp, $sp\n";
    fprintf(spm_code_file, "%s", str);
    int fp_offset = init_stack_record(codes);
    fprintf(spm_code_file, "  addi $sp, $sp, -%d\n", fp_offset);
}

/// @brief 参数压入栈，调用write，随后返回
/// @param op 参数OP
static void inline write_obj(Operand op) {
    if (op->kind == OP_CONSTANT) {
        fprintf(spm_code_file, "  li $a0, %d\n", op->u.value);
    } else {
        // 查找偏移量 -> 加载到a0
        int off = get_offset(op);
        fprintf(spm_code_file, "  lw $a0, %d($fp)\n", off);
    }
    const char *str =
        "  addi $sp, $sp, -4\n"  // 为返回地址分配空间
        "  sw $ra 0($sp)\n"      // 保存返回地址
        "  jal write\n"          // 调用write函数
        "  lw $ra 0($sp)\n"      // 恢复返回地址
        "  addi $sp, $sp, 4\n";  // 恢复栈指针
    fprintf(spm_code_file, "%s", str);
}

/// @brief 参数压入栈，调用read，随后返回
/// @param op 参数OP
static void inline read_obj(Operand op) {
    const char *str =
        "  addi $sp, $sp, -4\n"  // 为返回地址分配空间
        "  sw $ra 0($sp)\n"      // 保存返回地址
        "  jal read\n"           // 调用read函数
        "  lw $ra 0($sp)\n"      // 恢复返回地址
        "  addi $sp, $sp, 4\n";  // 恢复栈指针
    fprintf(spm_code_file, "%s", str);

    // 先获取一个临时寄存器，然后将v0写入临时寄存器，再将临时寄存器写入op
    reg_t *reg = get_reg();
    fprintf(spm_code_file, "  move %s, $v0\n", reg->name);
    fprintf(spm_code_file, "  sw %s, %d($fp)\n", reg->name, get_offset(op));
    // 还没想好要怎么把返回值传递给我的op
}

static void inline return_obj(Operand op) {
    // 判断Op的类型，如果是常数，那么直接将其写入v0，随后返回
    if (op->kind == OP_CONSTANT) {
        fprintf(spm_code_file, "  li $v0, %d\n", op->u.value);
    } else {
        // 查找偏移量 -> 加载到v0
        int off = get_offset(op);
        fprintf(spm_code_file, "  lw $v0, %d($fp)\n", off);
    }
    const char *str =
        "  move $sp, $fp\n"     // 恢复栈指针
        "  lw $fp, 0($sp)\n"    // 恢复帧指针
        "  addi $sp, $sp, 4\n"  // 恢复栈指针
        "  jr $ra\n";           // 返回
    fprintf(spm_code_file, "%s", str);
}

static void inline assign_obj(InterCode code) {
    int off_left = get_offset(code->u.assign.left);
    int off_right = get_offset(code->u.assign.right);
    reg_t *reg = get_reg();
    if (code->u.assign.ass_type == ASS_NORMAL) {
        load2reg(code->u.assign.right, reg);
        store2reg(code->u.assign.left, reg);
    } else if (code->u.assign.ass_type == ASS_GETADDR) {
        fprintf(spm_code_file, "  addi %s $fp %d\n", reg->name, off_right);
        fprintf(spm_code_file, "  sw %s %d($fp)\n", reg->name, off_left);
    } else if (code->u.assign.ass_type == ASS_GETVAL) {
        // x := *y
        load2reg(code->u.assign.right, reg);
    } else if (code->u.assign.ass_type == ASS_SETVAL) {
        // *x := y
        fprintf(spm_code_file, "  addi %s %d$(fp)\n", reg->name, off_left);  // reg(x)
        reg_t *tmp = get_reg();
        fprintf(spm_code_file, "  lw %s %d($fp)\n", reg->name, off_right);  // reg(y)
        fprintf(spm_code_file, "  sw %s 0(%s)\n", tmp->name, reg->name);    // sw reg(y) 0(reg(x))
    }
    RELEASE_REG(reg);
}

/// @brief 针对四则运算生成中间代码
/// r := x + y || x - y || x * y || x / y || &x + y
static void inline binary_obj(InterCode code) {
    reg_t *r1 = get_reg();
    reg_t *r2 = get_reg();
    reg_t *res = get_reg();

    Operand result = code->u.binop.result;
    Operand op1 = code->u.binop.op1;
    Operand op2 = code->u.binop.op2;

    if (code->kind != IR_ADDRADD)
        load2reg(op1, r1);
    else
        fprintf(spm_code_file, " addi %s $fp %d", get_offset(op1));

    load2reg(op2, r2);
    switch (code->kind) {
        case IR_ADDRADD:
        case IR_ADD: {
            fprintf(spm_code_file, "  add %s %s %s\n", res->name, r1->name, r2->name);
            break;
        }
        case IR_SUB: {
            fprintf(spm_code_file, "  sub %s %s %s\n", res->name, r1->name, r2->name);
            break;
        }
        case IR_MUL: {
            fprintf(spm_code_file, "  mul %s %s %s\n", res->name, r1->name, r2->name);
            break;
        }
        case IR_DIV: {
            fprintf(spm_code_file, "  div %s %s %s\n", res->name, r1->name, r2->name);
            break;
        }
        default:
            Panic("Invalid InterCode");
            break;
    }
    store2reg(result, res);
    RELEASE_REG(r1);
    RELEASE_REG(r2);
    RELEASE_REG(res);
}