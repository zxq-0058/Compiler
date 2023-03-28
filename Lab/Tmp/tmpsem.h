#ifndef _SEMANTICS_H
#define _SEMANTICS_H

#include <stdlib.h>
#include <stdarg.h>

#include "logger.h"

/**
 * TODO: 考虑符号表的设计
*/

typedef struct symbol {
    char *name; // 符号名字
    int type; // 符号类型
    int scope; // 符号作用域
    // TODO: 待完善
} Symbol;

typedef struct symbol_table {
    int scope; // 当前作用域
    struct symbol_table *prev; // 上一个作用域的符号表
    struct symbol_table *next; // 下一个作用域的符号表
    int size; // 符号表大小
    Symbol **symbols; // 符号表数组
} SymbolTable;



#endif