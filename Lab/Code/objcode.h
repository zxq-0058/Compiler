#ifndef _OBJCODE_H_
#define _OBJCODE_H_

#include "intercode.h"

#define inf 0x3f3f3f3f
#define RELEASE_REG(reg) (reg->is_free = 1)

static void inline fun_obj(InterCodes codes);
static void inline write_obj(Operand op);
static void inline read_obj(Operand op);
static void inline return_obj(Operand op);
static void inline assign_obj(InterCode code);
static void inline binary_obj(InterCode code);
static inline void ifgoto_obj(InterCode code);
static inline void call_obj(InterCode code);

// t0-t7()
typedef struct reg {
    char name[32];  // 寄存器名字
    int is_free;    // 是否被使用
} reg_t;

#define NUM_REGISTERS 8

#endif