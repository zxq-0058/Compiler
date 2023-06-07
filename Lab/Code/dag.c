#include "dag.h"

#include "basicblock.h"

#define MAX_VAR_NUM 1000
#define MAX_TEMP_NUM 1000
#define MAX_ADDR_NUM 2000

// [0, MAX_VAR_NUM) : 变量 v%d
// [MAX_VAR_NUM, MAX_VAR_NUM + MAX_TEMP_NUM) : 临时变量 t%d
// [MAX_VAR_NUM + MAX_TEMP_NUM, MAX_VAR_NUM + MAX_TEMP_NUM + MAX_ADDR_NUM) : 地址 *v%d
// 这里没有考虑&v%d，是因为&v%d只会出现在赋值语句的左边，而不会出现在右边
DAGNode map[MAX_VAR_NUM + MAX_TEMP_NUM + MAX_ADDR_NUM];

int node_num = 0;
DAGNode *dag = NULL;
extern char *print_operand(Operand op);
extern void print_intercode(InterCode code, FILE *fp);

/// @brief 初始化DAG
static inline void init_dag() {
    for (int i = 0; i < MAX_VAR_NUM + MAX_TEMP_NUM + MAX_ADDR_NUM; i++) {
        map[i] = NULL;
    }
    dag = NULL;
    node_num = 0;
}

static inline void free_dag() {
    for (int i = 0; i < node_num; i++) {
        free(dag[i]);
    }
    free(dag);
    dag = NULL;
    node_num = 0;
}

/// @brief 添加一个DAG节点到DAG数组中
static inline void add_node(DAGNode node) {
    if (dag == NULL) {
        dag = (DAGNode *)malloc(sizeof(DAGNode));

    } else {
        dag = (DAGNode *)realloc(dag, sizeof(DAGNode) * (node_num + 1));
    }
    node->id = node_num;
    dag[node_num++] = node;
}

/// @brief 为Operand op分配一个DAG节点(初始化)
static inline DAGNode alloc_node(int node_kind, Operand op) {
    DAGNode node = (DAGNode)malloc(sizeof(struct DAGNode_));
    Panic_on(node == NULL, "malloc failed");
    memset(node, 0, sizeof(struct DAGNode_));
    node->op = op;
    node->node_kind = node_kind;
    node->code = NULL;
    return node;
}

static inline int is_node_equal(DAGNode node1, DAGNode node2) {
    if (node1->op->kind != node2->op->kind) return 0;
    if (node1->op->kind == OP_CONSTANT) {
        return node1->op->u.value == node2->op->u.value;
    }
    return node1 == node2;
}

/// @brief 打印DAG（调试用）
void print_dag() {
#ifdef DEBUG_ON
    for (int i = 0; i < node_num; i++) {
        DAGNode node = dag[i];
        printf("Node %d: ", i);
        if (node->node_kind == NODE_LAEF) {
            printf("(叶子节点)OP: %s\n", print_operand(node->op));
        } else {
            printf("OP: %s\n", print_operand(node->op));
            if (node->child_num) printf("   Children:\n");
            for (int j = 0; j < node->child_num; j++) {
                printf("    OP: %s, %d\n", print_operand(node->children[j]->op), node->children[j]->id);
            }
        }
        printf("\n");
    }
#endif
}

/// @brief 将Operand op map到DAGNode node (v%d和t%d)
void chmap_op2node(Operand op, DAGNode node) {
    if (op->kind == OP_VARIABLE) {
        map[op->u.var_no] = node;
    } else if (op->kind == OP_TEMP) {
        map[MAX_VAR_NUM + op->u.tmp.tmp_no] = node;
    }
}

// 将Operand map到对应的DAG节点
// 对于变量和临时变量，如果没有对应的DAG节点，则创建一个，并添加到map中
// 对于常数，直接创建一个NODE_LAEF类型的DAG节点（不做map的修改）
DAGNode map_op2node(Operand op) {
    DAGNode ret = NULL;
    if (op->kind == OP_VARIABLE) {
        ret = map[op->u.var_no];
    } else if (op->kind == OP_TEMP) {
        ret = map[MAX_VAR_NUM + op->u.tmp.tmp_no];
    } else if (op->kind == OP_CONSTANT) {
        ret = alloc_node(NODE_LAEF, op);
    } else {
        Panic("Invalid Operand kind");
        return NULL;
    }
    if (ret == NULL) {
        ret = alloc_node(NODE_LAEF, op);
        if (op->kind == OP_VARIABLE) {
            map[op->u.var_no] = ret;
        } else if (op->kind == OP_TEMP) {
            map[MAX_VAR_NUM + op->u.tmp.tmp_no] = ret;
        }
    }
    return ret;
}

/// @brief 补丁函数，将*v%d和*t%d 或者&v%d map到DAGNode node
DAGNode map_op2node2(Operand op, char *type) {
    Panic_ON(strcmp(type, "*") != 0, "Invalid type");
    int offset = op->kind == OP_VARIABLE ? op->u.var_no : op->u.tmp.tmp_no + MAX_VAR_NUM;
    offset += MAX_VAR_NUM + MAX_TEMP_NUM;
    DAGNode ret = map[offset];
    if (ret == NULL) {
        ret = alloc_node(NODE_LAEF, op);
        map[offset] = ret;
    }
    // 每一个*v%d或者*t%d都会有一个默认的子节点 v%d或者t%d
    DAGNode child = map_op2node(op);
    child->star_parent[child->star_parent_num++] = ret;
    return ret;
}

/// @brief 补丁函数，将*v%d和*t%d 或者&v%d map到DAGNode node
void chmap_op2node2(Operand op, char *type, DAGNode node) {
    Panic_ON(strcmp(type, "*") != 0, "Invalid type");
    int offset = op->kind == OP_VARIABLE ? op->u.var_no : op->u.tmp.tmp_no + MAX_VAR_NUM;
    offset += MAX_VAR_NUM + MAX_TEMP_NUM;
    if (op->kind == OP_VARIABLE) {
        map[offset] = node;
    } else if (op->kind == OP_TEMP) {
        map[offset] = node;
    }
    DAGNode child = map_op2node(op);
    child->star_parent[child->star_parent_num++] = node;
}

/// @brief 将child添加到parent的孩子节点数组中
void dag_add_child(DAGNode parent, DAGNode child) {
    if (parent->child_num == 2) {
        Panic("DAGNode already has 2 children");
        return;
    }
    parent->children[parent->child_num++] = child;
    child->parent[child->parent_num++] = parent;
}

/// @brief 构建赋值语句的DAG节点 left := right
DAGNode build_assign_node(InterCode code) {
    /**
     * (1)获得right对应的DAG节点，如果没有则创建一个（NODE_LAEF类型）
     * (2)创建一个NODE_ASSIGN类型的DAG节点，将left map到该节点
     */
    Operand left = code->u.assign.left;
    Operand right = code->u.assign.right;
    DAGNode assign_node = NULL, child = NULL;
    switch (code->u.assign.ass_type) {
        case ASS_NORMAL: {  // x := y
            assign_node = alloc_node(NODE_NORMAL_ASS, left);
            child = map_op2node(right);
            chmap_op2node(left, assign_node);
            break;
        }
        case ASS_GETADDR: {  // x := &y
            assign_node = alloc_node(NODE_GETADDR_ASS, left);
            child = map_op2node(right);
            chmap_op2node(left, assign_node);
            break;
        }
        case ASS_GETVAL: {  // x := *y
            assign_node = alloc_node(NODE_GETVAL_ASS, left);
            child = map_op2node2(right, "*");
            chmap_op2node(left, assign_node);
            break;
        }
        case ASS_SETVAL:  // *x := y
            assign_node = alloc_node(NODE_SETVAL_ASS, left);
            child = map_op2node(right);
            chmap_op2node2(left, "*", assign_node);
            break;
        default:
            Panic("invaild!");
    }
    assign_node->code = code;
    dag_add_child(assign_node, child);
    return assign_node;
}

/// @brief 构建二元运算的DAG节点 left := op1 op op2
DAGNode build_bino_node(InterCode code) {
    Operand left = code->u.binop.result;
    Operand op1 = code->u.binop.op1;
    Operand op2 = code->u.binop.op2;
    int kind = -1;
    switch (code->kind) {
        case IR_ADD:
            kind = NODE_ADD;
            break;
        case IR_SUB:
            kind = NODE_SUB;
            break;
        case IR_MUL:
            kind = NODE_MUL;
            break;
        case IR_DIV:
            kind = NODE_DIV;
            break;
        default:
            Panic("Invalid InterCode kind");
            return NULL;
    }
    DAGNode bino_node = alloc_node(kind, left);

    DAGNode ch1 = NULL, ch2 = NULL;
    ch1 = map_op2node(op1);
    ch2 = map_op2node(op2);
    Panic_ON(ch1 == NULL || ch2 == NULL, "ch1 or ch2 is NULL");
    dag_add_child(bino_node, ch1);
    dag_add_child(bino_node, ch2);
    bino_node->code = code;

    chmap_op2node(left, bino_node);
    return bino_node;
}

/// @brief 构建IFGOTO语句的DAG节点
DAGNode build_ifgoto_node(InterCode code) {
    /**
     * IF x [relop] y GOTO z (保留节点)
     * 加边 : x -> ifgoto_node
     *        y -> ifgoto_node
     */
    DAGNode ifgoto_node = alloc_node(NODE_RESERVE, code);
    ifgoto_node->code = code;
    DAGNode child = map_op2node(code->u.ifgoto.x);
    dag_add_child(ifgoto_node, child);
    child = map_op2node(code->u.ifgoto.y);
    dag_add_child(ifgoto_node, child);
    return ifgoto_node;
}

DAGNode build_arg_node(InterCode code) {
    /**
     * ARG x（保留节点，意味着无论如何优化都不会删除当前指令，但是x可能会变化）
     * 加边 : x -> arg_node
     */
    DAGNode arg_node = alloc_node(NODE_RESERVE, code);
    arg_node->code = code;
    DAGNode child = map_op2node(code->u.one.op);
    dag_add_child(arg_node, child);
    return arg_node;
}

/// @brief 构建CALL语句的DAG节点
DAGNode build_call_node(InterCode code) {
    /**
     * x := CALL f（保留节点，意味着无论如何优化都不会删除当前指令，但是x可能会变化）
     */
    DAGNode call_node = alloc_node(NODE_RESERVE, code);
    call_node->code = code;
    call_node->op = code->u.two.left;
    chmap_op2node(code->u.two.left, call_node);
    return call_node;
}

/// @brief 构建WRITE语句的DAG节点
DAGNode build_write_node(InterCode code) {
    /**
     * WRITE x（保留节点，意味着无论如何优化都不会删除当前指令，但是x可能会变化）
     * 加边 : x -> write_node
     */
    DAGNode write_node = alloc_node(NODE_RESERVE, code);
    write_node->code = code;
    DAGNode child = map_op2node(code->u.one.op);
    dag_add_child(write_node, child);
    return write_node;
}

/// @brief 构建RETURN语句的DAG节点
DAGNode build_return_node(InterCode code) {
    DAGNode child = map_op2node(code->u.one.op);
    DAGNode return_node = alloc_node(NODE_RESERVE, child->op);
    dag_add_child(return_node, child);
    return_node->code = code;
    return return_node;
}

/// @brief 构建READ语句的DAG节点
DAGNode build_read_node(InterCode code) {
    /**
     * READ x（叶子节点，可能会被优化掉）
     * 不加边，直接将 x 绑定到 read_node
     */
    DAGNode read_node = alloc_node(NODE_LAEF, code);
    read_node->code = code;
    read_node->op = code->u.one.op;
    chmap_op2node(read_node->op, read_node);
    return read_node;
}

void buildDAG(BasicBlock block) {
    // 初始化DAG
    init_dag();
    Instruction instruction = NULL;
    for_each_in_list(instruction, struct Instruction_, node, &block->interCodes) {
        InterCode code = instruction->code;
        switch (code->kind) {
            case IR_ASSIGN: {
                DAGNode assign_node = build_assign_node(code);
                add_node(assign_node);
                break;
            }
            case IR_ADD:
            case IR_SUB:
            case IR_MUL:
            case IR_DIV: {
                DAGNode bino_node = build_bino_node(code);
                add_node(bino_node);
                break;
            }
            case IR_READ: {
                DAGNode read_node = build_read_node(code);
                add_node(read_node);
                break;
            }
            case IR_ADDRADD: {
                DAGNode addradd_node = (DAGNode)malloc(sizeof(struct DAGNode_));
                addradd_node->code = code;
                addradd_node->node_kind = NODE_RESERVE;
                DAGNode child1 = map_op2node(code->u.binop.op1);
                DAGNode child2 = map_op2node(code->u.binop.op2);
                dag_add_child(addradd_node, child1);
                dag_add_child(addradd_node, child2);
                add_node(addradd_node);
                break;
            }
            case IR_WRITE: {
                DAGNode write_node = build_write_node(code);
                add_node(write_node);
                break;
            }
            case IR_IFGOTO: {
                DAGNode ifgoto_node = build_ifgoto_node(code);
                add_node(ifgoto_node);
                break;
            }
            case IR_ARG: {
                DAGNode arg_node = build_arg_node(code);
                add_node(arg_node);
                break;
            }
            case IR_CALL: {
                DAGNode call_node = build_call_node(code);
                add_node(call_node);
                break;
            }
            case IR_PARAM: {
                DAGNode param_node = (DAGNode)malloc(sizeof(struct DAGNode_));
                param_node->code = code;
                param_node->node_kind = NODE_RESERVE;
                DAGNode child = map_op2node(code->u.one.op);
                dag_add_child(param_node, child);
                add_node(param_node);
                break;
            }
            case IR_RETURN: {
                DAGNode return_node = build_return_node(code);
                add_node(return_node);
                break;
            }
            default: {
                DAGNode empty_node = (DAGNode)malloc(sizeof(struct DAGNode_));
                empty_node->code = code;
                empty_node->node_kind = NODE_RESERVE;
                add_node(empty_node);
                break;
            }
        }
    }
    print_dag();
}

/// @brief 判断一个节点是否应该被删除
int should_node_delete(DAGNode node) {
    int ret = 0;
    if (node->is_deleted) return 1;
    if (node->node_kind == NODE_RESERVE) return 0;
    if (node->parent_num) {
        // 如果所有的父节点都被删除了，则该节点也应该被删除
        int flag = 1;
        for (int i = 0; i < node->parent_num; i++) {
            if (!node->parent[i]->is_deleted) {
                flag = 0;
                break;
            }
        }
        if (flag) ret = 1;
    }
    return ret;
}

/// @brief 将DAGNode导出为中间代码
InterCode export_node2ir(DAGNode node) {
    if (node->node_kind == NODE_RESERVE) {
        // 考虑到这里的节点中的变量可能已经发生变化，所以这里需要重新生成中间代码
        switch (node->code->kind) {
            case IR_LABEL:
            case IR_FUNCTION:
            case IR_GOTO:
            case IR_DEC:
                break;
            case IR_ADDRADD: {
                node->code->u.binop.op1 = node->children[0]->op;
                node->code->u.binop.op2 = node->children[1]->op;
                break;
            }
            case IR_ARG:
            case IR_PARAM:
            case IR_RETURN:
            case IR_WRITE: {
                node->code->u.one.op = node->children[0]->op;
                break;
            }
            case IR_IFGOTO: {
                node->code->u.ifgoto.x = node->children[0]->op;
                node->code->u.ifgoto.y = node->children[1]->op;
                break;
            }
        }
        return node->code;
    }
    switch (node->node_kind) {
        case NODE_RESERVE: {
            Panic("should not reached!");
        }
        case NODE_LAEF: {
            // 叶子节点：一般为常数或者变量
            break;
        }
        case NODE_NORMAL_ASS:
        case NODE_GETADDR_ASS:
        case NODE_GETVAL_ASS:
        case NODE_SETVAL_ASS: {
            node->code->u.assign.left = node->op;
            node->code->u.assign.right = node->children[0]->op;
            break;
        }
        case NODE_ADD:
        case NODE_SUB:
        case NODE_MUL:
        case NODE_DIV: {
            node->code->u.binop.result = node->op;
            node->code->u.binop.op1 = node->children[0]->op;
            node->code->u.binop.op2 = node->children[1]->op;
            break;
        }
        default:
            Panic("Invalid");
    }
    return node->code;
}

/// @brief 将dag导出为中间代码(For debugging)
/// @param out
void export_dag2ir(BasicBlock out) {
    del_all_intercodes(out);
    for (int i = 0; i < node_num; i++) {
        DAGNode node = dag[i];
        if (node->is_deleted) {
            continue;
        }
        InterCode code = export_node2ir(node);
        if (code) {
            append_intercode(out, code);
        }
    }
}

/// @brief 两个节点是否能被等价为一个节点
/// @return
int can_reduce2same(DAGNode node1, DAGNode node2) {
    if (node1->node_kind == NODE_RESERVE || node2->node_kind == NODE_RESERVE) return 0;
    if (node1->node_kind == NODE_SETVAL_ASS || node2->node_kind == NODE_SETVAL_ASS) return 0;
    if (node1->node_kind != node2->node_kind) return 0;
    if (node1->op->kind == OP_VARIABLE || node2->op->kind == OP_VARIABLE) return 0;
    if (node1->node_kind == NODE_LAEF) {
        return is_node_equal(node1, node2);
    } else {
        if (node1->child_num != node2->child_num) return 0;
        for (int i = 0; i < node1->child_num; i++) {
            if (!is_node_equal(node1->children[i], node2->children[i])) return 0;
        }
        return 1;
    }
}

/// @brief 优化基本块(一趟), 如果本次优化有删除节点，则返回1，否则返回0
int exp_optimize_pass() {
    // 优化DAG
    int flag = 0;
    for (int i = 0; i < node_num; i++) {
        DAGNode node1 = dag[i];
        if (node1->is_deleted || node1->node_kind == NODE_RESERVE) continue;
        if (should_node_delete(node1)) {
            node1->is_deleted = 1;
            flag = 1;
            continue;
        }
        for (int j = i + 1; j < node_num; j++) {
            DAGNode node2 = dag[j];
            if (node2->is_deleted) continue;
            if (can_reduce2same(node1, node2)) {
                // 将node2从DAG中删除
                flag = 1;
                node2->is_deleted = 1;
                // 将node2的父节点指向node1
                for (int k = 0; k < node2->parent_num; k++) {
                    DAGNode parent = node2->parent[k];
                    for (int l = 0; l < parent->child_num; l++) {
                        if (parent->children[l] == node2) {
                            parent->children[l] = node1;
                        }
                    }
                }
                // 将start_parent换成node1
                for (int k = 0; k < node2->star_parent_num; k++) {
                    DAGNode star_parent = node2->star_parent[k];
                    star_parent->op = node1->op;
                }
            }
        }
    }
    return flag;
}
/// @brief 公共子表达式优化
void common_exp_optimize(BasicBlock block) {
    // 构建DAG
    buildDAG(block);
    // 优化DAG(多趟)
    while (exp_optimize_pass())
        ;
    export_dag2ir(block);
}