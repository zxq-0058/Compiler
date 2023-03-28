#ifndef _SEMANTICS_H
#define _SEMANTICS_H

#include <stdlib.h>
#include <stdarg.h>

#include "logger.h"

// =========================================================== //
/**
 * 数据类型的定义
 */
typedef struct Type_ *Type;
typedef struct FieldList_ *FieldList;

struct Type_
{
    enum
    {
        BASIC,
        ARRAY,
        STRUCTURE
    } kind;
    union
    {
        int basic; // 基本类型
        struct
        {
            Type elem; // 数组类型
            int size;  // 数组的大小
        } array;
        FieldList structure;
    } u;
};

struct FieldList_
{
    char *name;     // 域的名字
    Type type;      // 域的类型
    FieldList tail; // 下一个域
};

// =========================================================== //
/**
 * 符号表的定义
 */
typedef struct symbol
{
    char *name; // 符号名字
    int type;   // 符号类型
    int scope;  // 符号作用域
    // TODO: add some info
} Symbol;

typedef struct symbol_table_entry
{
    Symbol *symbol;
    struct symbol_table_entry *next;
} SymbolTableEntry;

typedef struct symbol_table
{
    int scope;                 // 当前作用域, 0 全局作用域
    struct symbol_table *prev; // 上一个作用域的符号表
    struct symbol_table *next; // 下一个作用域的符号表
    int size;                  // 符号表大小
    SymbolTableEntry **table;  // 哈希表
} SymbolTable;

// 查找符号表中的符号
Symbol *lookup_symbol(SymbolTable *table, char *name);

// 向符号表中添加符号
void add_symbol(SymbolTable *table, Symbol *sym);

// 进入新的作用域
SymbolTable *enter_scope(SymbolTable *table);

// 退出当前作用域
SymbolTable *exit_scope(SymbolTable *table);

// 初始化符号表
SymbolTable *init_symbol_table(int size);

// 释放符号表
void free_symbol_table(SymbolTable *table);

#endif
