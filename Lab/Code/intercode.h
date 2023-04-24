#ifndef _INTERCODE_H_
#define _INTERCODE_H_

#include "semantics.h"
#include "syntax.h"

typedef struct Operand_* Operand;
typedef struct InterCode_* InterCode;
typedef struct InterCodes_* InterCodes;

struct Operand_ {
    enum {
        OP_VARIABLE,  // operand is a variable(源代码里面的变量)
        OP_TEMP,      // operand is a temporary variable(临时变量)
        OP_CONSTANT,  // operand is a constant(整型常数)
        OP_ADDRESS,   // operand is the address of a variable(地址，结构体或者是数组)
        OP_FUNCTION,  // operand is a function(函数)
    } kind;
    union {
        int var_no;
        struct {
            int tmp_no;
            int tmp_addr;  // 0:表示t{tmp_no}存放的是值, 1:表示t{tmp_no}存放的是地址
        } tmp;
        int value;
        char* relop;
        char* func_name;
    } u;
};

struct InterCode_ {
    enum {
        IR_LABEL,     // LABEL x
        IR_FUNCTION,  // FUNCTION f
        IR_ASSIGN,    // x := y, x := &y, x := *y, *x := y
        IR_ADD,       // x := y + z
        IR_ADDRADD,   // x := &y + z
        IR_SUB,       // x := y - z
        IR_MUL,       // x := y * z
        IR_DIV,       // x := y / z
        IR_GOTO,      // GOTO x
        IR_IFGOTO,    // IF x [relop] y GOTO z
        IR_RETURN,    // RETURN x
        IR_DEC,       // DEC x [size]
        IR_ARG,       // ARG x
        IR_CALL,      // x := CALL f
        IR_PARAM,     // PARAM x
        IR_READ,      // READ x
        IR_WRITE      // WRITE x
    } kind;
    union {
        struct {
            Operand op;  // label, function, variable, constant, go, return, read, write, arg, param
        } one;

        /**
         * left := CALL right (函数调用)
         */
        struct {
            Operand left, right;
        } two;
        struct {
            enum { ASS_NORMAL, ASS_GETADDR, ASS_GETVAL, ASS_SETVAL } ass_type;
            Operand left, right;
            /**
             * 0: normal assignment -> x := y
             * 1: get address -> x := &y
             * 2: get value -> x := *y
             * 3: set value -> *x := y
             */
        } assign;
        struct {
            /**
             * + - * / 四则运算 以及 地址偏移（x := &y + z）
             */
            Operand result, op1, op2;
        } binop;
        struct {
            /**
             * IF x [relop] y GOTO z
             */
            Operand x, y, z;
            char* relop;
        } ifgoto;
        struct {
            /**
             * DEC x [size]
             */
            Operand op;
            int size;
        } dec;
    } u;
};

struct InterCodes_ {
    InterCode code;
    InterCodes prev, next;
};

#endif
