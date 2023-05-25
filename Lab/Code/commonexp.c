// #include "commonexp.h"

// #include "basicblock.h"

// /// @brief 判断两个操作数是否相等
// int operand_equal(Operand op1, Operand op2) {
//     if (op1 == NULL && op2 == NULL) {
//         return 1;
//     }
//     if (op1 == NULL || op2 == NULL) {
//         return 0;
//     }
//     if (op1->kind != op2->kind) {
//         return 0;
//     }
//     switch (op1->kind) {
//         case OP_VARIABLE:
//             return op1->u.var_no == op2->u.var_no;
//         case OP_CONSTANT:
//             return op1->u.value == op2->u.value;
//         case OP_TEMP:
//             return op1->u.tmp.tmp_no == op2->u.tmp.tmp_no;
//         case OP_LABEL:
//             return op1->u.var_no == op2->u.var_no;
//         case OP_FUNCTION:
//             return strcmp(op1->u.func_name, op2->u.func_name) == 0;
//         default:
//             return 0;
//     }
// }

// int is_expression_in_block(Expression *expr, BasicBlock *block) {
//     ListElem *elem;
//     for (elem = block->expressions->head; elem != NULL; elem = elem->next) {
//         Expression *cur_expr = (Expression *)elem->data;
//         if (cur_expr->kind == expr->kind && operand_equal(cur_expr->op1, expr->op1) &&
//             operand_equal(cur_expr->op2, expr->op2)) {
//             return 1;
//         }
//     }
//     return 0;
// }

// void local_common_subexpression_elimination(BasicBlock *block) {
//     InterCode code;
//     for (code = block->head; code != block->tail->next; code = code->next) {
//         if (code->kind >= IR_ADD && code->kind <= IR_DIV) {
//             // 当前指令是一个二元操作符
//             Expression expr;
//             expr.kind = code->kind;
//             expr.op1 = code->u.binop.op1;
//             expr.op2 = code->u.binop.op2;
//             expr.result = code->u.binop.result;

//             if (is_expression_in_block(&expr, block)) {
//                 // 如果当前表达式已经在基本块中计算过，用之前计算过的结果替换当前指令的结果
//                 // 注意：这里假设之前计算过的结果存储在一个临时变量中，你需要根据你的具体实现进行相应的修改
//                 code->u.binop.result = get_previous_result(&expr, block);
//             } else {
//                 // 如果当前表达式没有在基本块中计算过，将其添加到表达式列表中
//                 add_expression_to_block(&expr, block);
//             }
//         }
//     }
// }

// void createDAG(BasicBlock bb, DAGNode **dag, int *dag_size) {
//     InterCode *code = bb->code;
//     int code_count = bb->code_count;
//     DAGNode *nodes[MAX_CODE_LENGTH];
//     int node_count = 0;
//     for (int i = 0; i < code_count; i++) {
//         InterCode c = code[i];
//         if (c->kind == IR_ASSIGN) {
//             Operand left = c->u.assign.left;
//             Operand right = c->u.assign.right;
//             if (right->kind != OP_CONSTANT) {
//                 DAGNode *node = (DAGNode *)malloc(sizeof(DAGNode));
//                 node->op = c->kind;
//                 node->operand = left;
//                 node->parents = (DAGNode **)malloc(sizeof(DAGNode *) * node_count);
//                 node->parent_count = 0;
//                 for (int j = 0; j < node_count; j++) {
//                     DAGNode *parent = nodes[j];
//                     if (parent->op == c->kind && operandEqual(parent->operand, right)) {
//                         node->parents[node->parent_count++] = parent;
//                         break;
//                     }
//                 }
//                 if (node->parent_count == 0) {
//                     nodes[node_count++] = node;
//                 } else {
//                     free(node);
//                 }
//             }
//         } else {
//             DAGNode *node = (DAGNode *)malloc(sizeof(DAGNode));
//             node->op = c->kind;
//             node->operand = NULL;
//             node->parents = (DAGNode **)malloc(sizeof(DAGNode *) * node_count);
//             node->parent_count = 0;
//             nodes[node_count++] = node;
//         }
//     }
//     *dag = (DAGNode **)malloc(sizeof(DAGNode *) * node_count);
//     *dag_size = node_count;
//     for (int i = 0; i < node_count; i++) {
//         (*dag)[i] = nodes[i];
//     }
// }

// int hashNode(DAGNode *node) {
//     int hash = node->op;
//     if (node->operand != NULL) {
//         hash = hash * 31 + node->operand->kind;
//         if (node->operand->kind == OP_VARIABLE) {
//             hash = hash * 31 + node->operand->u.var_no;
//         } else {
//             hash = hash * 31 + node->operand->u.value;
//         }
//     }
//     for (int i = 0; i < node->parent_count; i++) {
//         hash = hash * 31 + (unsigned long)node->parents[i];
//     }
//     return hash;
// }

// void replaceCommonSubexpressions(DAGNode **dag, int dag_size) {
//     DAGNode *hash_table[HASH_SIZE] = { NULL };
//     for (int i = 0; i < dag_size; i++) {
//         DAGNode *node = dag[i];
//         int hash = hashNode(node) % HASH_SIZE;
//         while (hash_table[hash] != NULL) {
//             DAGNode *existing_node = hash_table[hash];
//             if (existing_node->op == node->op && operandEqual(existing_node->operand, node->operand) &&
//             existing_node->parent_count == node->parent_count) {
//                 int j;
//                 for (j = 0; j < node->parent_count; j++) {
//                     if (!isNodeInParents(node->parents[j], existing_node)) {
//                         break;
//                     }
//                 }
//                 if (j == node->parent_count) {
//                     // Found common subexpression
//                     Operand temp = newTemp();
//                     InterCode assign = newInterCode(IR_ASSIGN, temp, node->operand, ASS_NORMAL);
//                     insertInterCodeBefore(bb, i, assign);
//                     node->operand = temp;
//                     node->op = IR_ASSIGN;
//                     for (int k = 0; k < node->parent_count; k++) {
//                         replaceNodeInParents(node, existing_node, node->parents[k]);
//                     }
//                     break;
//                 }
//             }
//             hash = (hash + 1) % HASH_SIZE;
//         }
//         if (hash_table[hash] == NULL) {
//             hash_table[hash] = node;
//         }
//     }
// }