#ifndef _SEMANTICS_H
#define _SEMANTICS_H

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "syntax.h"

// =========================================================== //
/**
 * 数据类型的定义
 */
typedef struct Type_ *Type;
typedef struct FieldList_ *FieldList;

enum BasicType { INT, FLOAT } BasicType;

struct Type_ {
    enum { BASIC, ARRAY, STRUCTURE } kind;
    union {
        int basic;  // 基本类型
        struct {
            Type elem;  // 数组类型
            int size;   // 数组的大小
        } array;
        FieldList structure;
    } u;
};

struct FieldList_ {
    char *name;      // 域的名字
    Type type;       // 域的类型
    FieldList tail;  // 下一个域
};

// =========================================================== //
/**
 * 符号表的定义
 */
enum SymType { VAR, FUNC };

typedef struct ParamList_ *ParamList;

struct ParamList_ {
    char *name;      // 参数名字
    Type type;       // 参数类型
    ParamList next;  // 下一个参数
};

typedef struct symbol {
    char *name;                    // 符号名字
    enum SymType symType;          // 符号类型
    int scope;                     // 符号作用域
    Type vartype;                  // type=VAR时有效
    Type returnType;               // type=FUNC时有效，函数返回值类型
    struct ParamList_ *paramList;  // type=FUNC时有效，函数参数列表
    int isFundef;                  // type=FUNC时有效，函数是否定义过
} Symbol;

typedef struct symbol_table_entry {
    Symbol *symbol;
    struct symbol_table_entry *next;
} SymbolTableEntry;

typedef struct symbol_table {
    int scope;                  // 当前作用域, 0 全局作用域
    struct symbol_table *prev;  // 上一个作用域的符号表
    struct symbol_table *next;  // 下一个作用域的符号表
    int size;                   // 符号表大小
    SymbolTableEntry **table;   // 哈希表
} SymbolTable;

// 查找符号表中的符号
Symbol *lookup_symbol(SymbolTable *table, char *name, int flag, enum SymType symType);

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

void sem_error(int type, int lineno, const char *mssg);

/* Specifiers */
Type specifier_handler(ASTNode *specifier);
Type struct_specifier_handler(ASTNode *str_specifier);

#endif
