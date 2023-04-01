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
enum SymType { VAR, FUNC, STRUCT };

typedef struct ParamList_ *ParamList;

struct ParamList_ {
    char *name;      // 参数名字
    Type type;       // 参数类型
    ParamList next;  // 下一个参数
};

typedef struct ArgList_ *ArgList;
struct ArgList_ {
    Type type;     // 实参的类型
    ArgList next;  // 下一个参数
};

typedef struct symbol {
    char *name;            // 符号名字
    enum SymType symType;  // 符号类型： =VAR, FUNC, STRUCTURE
    int scope;             // 符号作用域
    int lineno;            // 符号所在行号，只是为了更好的报错
    //------------------------//
    Type vartype;    // type=VAR时有效
    int isAssigned;  // type=VAR时有效，是否已经被赋值
    //------------------------//
    Type returnType;               // type=FUNC时有效，函数返回值类型
    struct ParamList_ *paramList;  // type=FUNC时有效，函数参数列表
    int isFundef;                  // type=FUNC时有效，函数是否定义过
    // ----------------------- //
    Type structType;  // type=STRUCTURE时有效，结构体类型
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

static int isEqualType(Type t1, Type t2);
Type createBasicType(const char *basicTypeStr);
Type createArrayType(Type elem, int size);
FieldList createFieldList(char *name, Type type, FieldList tail);
Type createStructureType(FieldList fields);

void appendParamList(ParamList *paramList, char *name, Type type);
static int isEqualParamList(ParamList p1, ParamList p2);
void appendArgList(ArgList *arglist, Type type);
int compareArgListParamList(ArgList argList, ParamList paramList);

static inline Symbol *createVarSymbol(char *name, int scope, Type varType, int lineno, int isAssigned);
static inline Symbol *createFunSymbol(char *fname, int scope, Type returnType, ParamList params, int isFundef,
                                      int lineno);

// 计算符号名字的哈希值
static unsigned int hash_symbol_name(const char *name);

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

/* High-level definitions */
void program_handler(ASTNode *program);
void extDefList_handler(ASTNode *extdeflist);
void extDef_handler(ASTNode *extdef);
void extDecList_handler(ASTNode *extdeclist, Type type);
void function_dec_handler(ASTNode *func_dec);
void function_def_handler(ASTNode *func_def);

/* Specifiers */
Type specifier_handler(ASTNode *specifier);
Type structSpecifier_handler(ASTNode *str_specifier);

/* Declarators */

Symbol *varDec_handler(ASTNode *vardec, Type type, int isAssigned);
void varList_handler(ASTNode *varlist, ParamList *params);
Symbol *paramDec_hanlder(ASTNode *paramdec);
ParamList funDec_handler(ASTNode *fundec);

/* Statements */
// 考虑到需要处理return语句，因此需要上层调用者FuncDec_hanlder()告诉CompSt合法的返回类型
void compst_handler(ASTNode *compst, Type return_type);
void stmtList_handler(ASTNode *stmtlist, Type return_type);
void stmt_handler(ASTNode *stmt, Type return_type);

/* Local Definitions */
void defList_handler(ASTNode *deflist, FieldList *fields);
void def_handler(ASTNode *def, FieldList *fields);
void decList_handler(ASTNode *declist, Type type, FieldList *fields);
Symbol *dec_handler(ASTNode *dec, Type type);

/* Expressions */

typedef struct expr {
    int onlyRight;  // 是否仅仅是右值表达式
    Type expType;   // 表达式类型
} ExprRet;

ExprRet exp_handler(ASTNode *exp);
void args_handler(ASTNode *args, ArgList *arglist);

/* Init */
void init();

/* Error Reporting */
void sem_error(int type, int lineno, const char *format, ...);

#endif