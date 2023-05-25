#ifndef _COMMONEXP_H_
#define _COMMONEXP_H_
#include "intercode.h"

typedef struct Expression_ {
    int kind;        // 表达式的类型，与中间代码的类型相对应（例如，IR_ADD、IR_SUB等）
    Operand op1;     // 操作数1
    Operand op2;     // 操作数2
    Operand result;  // 结果
} Expression;

#endif  // _COMMONEXP_H_