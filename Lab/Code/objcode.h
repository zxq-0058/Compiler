#ifndef _OBJCODE_H_
#define _OBJCODE_H_

#include "intercode.h"

static void inline fun_obj(char *fun_name);
static void inline write_obj(Operand op);

// t0-t7()
typedef struct reg {
    char name[32];  // 寄存器名字
    int is_free;    // 是否被使用
} reg_t;

#define NUM_REGISTERS 8

#endif