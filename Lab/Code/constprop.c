#include "constprop.h"

#include "intercode.h"

/// @brief 创建一个新的常量操作数(copy from intercode.c)
static inline Operand new_const(int value) {
    Operand constant = (Operand)malloc(sizeof(struct Operand_));
    constant->kind = OP_CONSTANT;
    constant->u.value = value;
    return constant;
}

/// @brief 将一条中间代码转换为赋值语句
static inline void alter2assign(InterCode code, Operand left, Operand right) {
    code->kind = IR_ASSIGN;
    code->u.assign.ass_type = ASS_NORMAL;
    code->u.assign.left = left;
    code->u.assign.right = right;
}

/// @brief 替换OP : 将old操作数替换为new操作数，需要注意的是，如果old操作数在左侧出现，说明此时常量传播结束
void replace_operand(InterCodes start, InterCodes end, Operand old, Operand new) {
    InterCodes p = start;
    while (p != end) {
        InterCode code = p->code;
        if (code->kind == IR_ASSIGN) {  // 处理赋值语句 -> left := right
            Operand left = code->u.assign.left;
            Operand right = code->u.assign.right;
            if (operand_equal(old, left)) {  // 此时常量传播结束
                return;
            }
            if (operand_equal(right, old)) {  // 将右操作数替换为新的操作数
                code->u.assign.right = new;
            }
        } else if (isBinop(code->kind)) {  // 处理二元运算
            Operand result = code->u.binop.result;
            Operand op1 = code->u.binop.op1;
            Operand op2 = code->u.binop.op2;
            if (operand_equal(result, old)) {  // 此时常量传播结束
                return;
            }
            if (operand_equal(op1, old)) {  // 将左操作数替换为新的操作数
                code->u.binop.op1 = new;
            }
            if (operand_equal(op2, old)) {  // 将右操作数替换为新的操作数
                code->u.binop.op2 = new;
            }
        } else if (code->kind == IR_IFGOTO) {
            Operand op1 = code->u.ifgoto.x;
            Operand op2 = code->u.ifgoto.y;
            if (operand_equal(op1, old)) {  // 将左操作数替换为新的操作数
                code->u.ifgoto.x = new;
            }
            if (operand_equal(op2, old)) {  // 将右操作数替换为新的操作数
                code->u.ifgoto.y = new;
            }
        } else if (code->kind == IR_RETURN) {
            Operand op = code->u.one.op;
            if (operand_equal(op, old)) {  // 将操作数替换为新的操作数
                code->u.one.op = new;
            }
        } else if (code->kind == IR_READ) {
            Operand op = code->u.one.op;
            if (operand_equal(op, old)) {  // 将操作数替换为新的操作数
                code->u.one.op = new;
            }
        } else if (code->kind == IR_WRITE) {
            Operand op = code->u.one.op;
            if (operand_equal(op, old)) {  // 将操作数
                code->u.one.op = new;
            }
        }
        p = p->next;
    }
}

// 常量传播和常量折叠 [start, end)
void const_propagate(InterCodes start, InterCodes end) {
    InterCodes p = start;
    while (p != end) {
        InterCode code = p->code;
        if (code->kind == IR_ASSIGN) {  // 处理赋值语句
            Operand left = code->u.assign.left;
            Operand right = code->u.assign.right;
            if (right->kind == OP_CONSTANT) {  // 处理常量传播
                // p->code->u.assign.left = right;
                replace_operand(p->next, end, left, right);
            }
        } else if (code->kind == IR_ADD || code->kind == IR_SUB || code->kind == IR_MUL ||
                   code->kind == IR_DIV) {  // 处理常量传播和常量折叠
            Operand result = code->u.binop.result;
            Operand op1 = code->u.binop.op1;
            Operand op2 = code->u.binop.op2;
            Operand replace = NULL;
            if (code->kind == IR_ADD) {
                if (op1->kind == OP_CONSTANT && op2->kind == OP_CONSTANT) {
                    replace = new_const(op1->u.value + op2->u.value);
                    alter2assign(code, result, replace);
                } else if (op1->kind == OP_CONSTANT && op1->u.value == 0) {
                    replace = op2;
                    alter2assign(code, result, op2);
                } else if (op2->kind == OP_CONSTANT && op2->u.value == 0) {
                    replace = op1;
                    alter2assign(code, result, op1);
                }
            } else if (code->kind == IR_SUB) {
                if (op1->kind == OP_CONSTANT && op2->kind == OP_CONSTANT) {
                    replace = new_const(op1->u.value - op2->u.value);
                    alter2assign(code, result, replace);
                } else if (op2->kind == OP_CONSTANT && op2->u.value == 0) {
                    replace = op1;
                    alter2assign(code, result, op1);
                }
            } else if (code->kind == IR_MUL) {
                if (op1->kind == OP_CONSTANT && op2->kind == OP_CONSTANT) {
                    replace = new_const(op1->u.value * op2->u.value);
                    alter2assign(code, result, replace);
                } else if (op1->kind == OP_CONSTANT && op1->u.value == 1) {
                    replace = op2;
                    alter2assign(code, result, op2);
                } else if (op2->kind == OP_CONSTANT && op2->u.value == 1) {
                    replace = op1;
                    alter2assign(code, result, op1);
                }
            } else if (code->kind == IR_DIV) {
                if (op1->kind == OP_CONSTANT && op2->kind == OP_CONSTANT) {
                    Panic_on(op2->u.value == 0, "Divide by zero!");
                    replace = new_const(op1->u.value / op2->u.value);
                    alter2assign(code, result, replace);
                } else if (op1->kind == OP_CONSTANT && op1->u.value == 0) {
                    replace = new_const(0);
                    alter2assign(code, result, replace);
                } else if (op2->kind == OP_CONSTANT && op2->u.value == 1) {
                    replace = op1;
                    alter2assign(code, result, op1);
                }
            }
            if (replace != NULL) {
                replace_operand(p->next, end, result, replace);
            }
        }
        p = p->next;
    }
}

// Path: Code/constprop.c