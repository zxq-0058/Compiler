#include "semantics.h"

#include "logger.h"

static SymbolTable *current_table = NULL;

// 判断两个Type是否一致
static int isEqualType(Type t1, Type t2) {
    if (t1 == t2) {
        return 1;  // 如果两个指针本身相等，它们指向的内容也一定相等
    }

    if (t1 == NULL || t2 == NULL) {
        return 0;  // 如果其中一个指针为 NULL，它们不相等
    }

    if (t1->kind != t2->kind) {
        return 0;  // 如果两个 Type 的 kind 不同，它们不相等
    }

    switch (t1->kind) {
        case BASIC:
            return t1->u.basic == t2->u.basic;  // 如果两个基本类型相等，它们相等
        case ARRAY:
            return t1->u.array.size == t2->u.array.size &&
                   isEqualType(t1->u.array.elem, t2->u.array.elem);  // 如果两个数组的元素类型和大小都相等，它们相等
        case STRUCTURE:
            // 递归比较结构体的每一个域是否相等
            for (FieldList f1 = t1->u.structure, f2 = t2->u.structure; f1 != NULL && f2 != NULL;
                 f1 = f1->tail, f2 = f2->tail) {
                if (!isEqualType(f1->type, f2->type)) {
                    // 注意这里没有比较结构体域的名字（详细见讲义“结构体等价”）
                    return 0;
                }
            }
            // 如果两个结构体的所有域都相等，它们相等
            return (t1->u.structure == NULL && t2->u.structure == NULL) ||
                   (t1->u.structure != NULL && t2->u.structure != NULL);
        default:
            return 0;  // 未知类型，不相等
    }
}

Type createBasicType(const char *basicTypeStr) {
    Type newType = (Type)malloc(sizeof(struct Type_));
    newType->kind = BASIC;

    if (strcmp(basicTypeStr, "INT") == 0) {
        newType->u.basic = INT;
    } else if (strcmp(basicTypeStr, "FLOAT") == 0) {
        newType->u.basic = FLOAT;
    } else {
        // Error handling
        free(newType);
        Panic("Invalid Basic Type");
        return NULL;
    }

    return newType;
}

Type createArrayType(Type elem, int size) {
    Type newType = (Type)malloc(sizeof(struct Type_));
    newType->kind = ARRAY;
    newType->u.array.elem = elem;
    newType->u.array.size = size;
    return newType;
}

Type createStructureType(FieldList fields) {
    Type newType = (Type)malloc(sizeof(struct Type_));
    newType->kind = STRUCTURE;
    newType->u.structure = fields;
    return newType;
}

FieldList create_FieldList(char *name, Type type, FieldList tail) {
    FieldList newField = (FieldList)malloc(sizeof(struct FieldList_));
    newField->name = name;
    newField->type = type;
    newField->tail = tail;
    return newField;
}

ParamList createParamList(char *name, Type type, ParamList next) {
    ParamList paramList = (ParamList)malloc(sizeof(struct ParamList_));
    paramList->name = name;
    paramList->type = type;
    paramList->next = next;
    return paramList;
}

static inline Symbol *createVarSymbol(char *name, int scope, Type varType) {
    Symbol *varsym = (Symbol *)malloc(sizeof(Symbol));
    varsym->name = name;
    varsym->symType = VAR;
    varsym->scope = scope;
    varsym->vartype = varType;
    return varsym;
}

static inline Symbol *createFunSymbol(char *fname, int scope, Type returnType, ParamList params, int isFundef) {
    Symbol *funsym = (Symbol *)malloc(sizeof(Symbol));
    funsym->name = fname;
    funsym->scope = scope;
    funsym->symType = FUNC;
    funsym->returnType = returnType;
    funsym->paramList = params;
    funsym->isFundef = isFundef;
}
// 计算符号名字的哈希值
static unsigned int hash_symbol_name(const char *name) {
    unsigned int hash = 0, i;
    for (; *name; ++name) {
        hash = (hash << 2) + *name;
        if (i = hash & ~0x3fff) hash = (hash ^ (i >> 12)) & 0x3fff;
    }
    return hash;
    // 后续拿到hash值，需要对哈希表大小mod
}

// 查找符号表中的符号: flag = 0，仅仅限于当前的作用域; flag=1，支持向上查找
Symbol *lookup_symbol(SymbolTable *table, char *name, int flag, enum SymType symType) {
    unsigned int index = hash_symbol_name(name) % table->size;
    do {
        SymbolTableEntry *entry = table->table[index];  // 这里应该是一个链表
        while (entry != NULL) {
            Symbol *sym = entry->symbol;
            Panic_on(sym == NULL, "Symbol NULL!");
            if (strcmp(sym->name, name) == 0 && symType == sym->symType) {
                return sym;
            }
            entry = entry->next;
        }
        table = table->prev;  // 切换到上一层的作用域
    } while (flag && table != NULL);
    return NULL;  // 符号未定义
}

// 向符号表中添加符号
void add_symbol(SymbolTable *table, Symbol *sym) {
    unsigned int index = hash_symbol_name(sym->name) % table->size;
    // 链表插入过程
    SymbolTableEntry *entry = (SymbolTableEntry *)malloc(sizeof(SymbolTableEntry));
    entry->symbol = sym;
    entry->next = table->table[index];
    table->table[index] = entry;
}

// 进入新的作用域
SymbolTable *enter_scope(SymbolTable *table) {
    SymbolTable *new_table = (SymbolTable *)malloc(sizeof(SymbolTable));
    new_table->scope = table->scope + 1;
    new_table->prev = table;
    new_table->next = NULL;
    new_table->size = table->size;
    new_table->table = (SymbolTableEntry **)calloc(table->size, sizeof(SymbolTableEntry *));
    table->next = new_table;
    return new_table;
}

// 退出当前作用域
SymbolTable *exit_scope(SymbolTable *table) {
    SymbolTable *prev_table = table->prev;
    for (int i = 0; i < table->size; i++) {
        SymbolTableEntry *entry = table->table[i];
        while (entry != NULL) {
            SymbolTableEntry *next = entry->next;
            free(entry);
            entry = next;
        }
    }
    free(table->table);
    free(table);
    prev_table->next = NULL;
    return prev_table;
}

// 初始化符号表
SymbolTable *init_symbol_table(int size) {
    SymbolTable *table = (SymbolTable *)malloc(sizeof(SymbolTable));
    table->scope = 0;
    table->prev = NULL;
    table->next = NULL;
    table->size = size;
    table->table = (SymbolTableEntry **)calloc(size, sizeof(SymbolTableEntry *));
    return table;
}

// 释放符号表
void free_symbol_table(SymbolTable *table) {
    while (table != NULL) {
        SymbolTable *next_table = table->next;
        exit_scope(table);
        table = next_table;
    }
}

/**
 * 语义分析之前的初始化操作
 */
void init() {
    current_table = (SymbolTable *)malloc(sizeof(SymbolTable));
    current_table->scope = 0;
    current_table->prev = current_table->next = NULL;
    current_table->size = 128;
    current_table->table = (SymbolTableEntry **)calloc(current_table->size, sizeof(SymbolTableEntry *));
}

// ========================================================================== //
/* High-level definitions */
void program_handler(ASTNode *program) { extDefList_handler(program->child_list[0]); }

void extDefList_handler(ASTNode *extdeflist) {
    if (extdeflist->child_num == 1) {
        extDef_handler(extdeflist->child_list[0]);
    } else {
        extDef_handler(extdeflist->child_list[0]);
        extDefList_handler(extdeflist->child_list[1]);
    }
}

void extDef_handler(ASTNode *extdef) {
    if (extdef->child_num == 3) {
        // Specifier ExtDecList SEMI(全局变量声明)
        Type type = specifier_handler(extdef->child_list[0]);
        extDecList_handler(extdef->child_list[1], type);
    } else if (extdef->child_num == 1) {
        // Specifier SEMI(结构体变量声明)
        Type type = specifier_handler(extdef->child_list[0]);
    } else {
        char *chd_name = extdef->child_list[0]->node_name;
        Panic_on(extdef->child_num != 1, "ExtDef");
        if (strcmp("Function_Declaration", chd_name) == 0) {
            // Function_Declaration(函数声明)
            function_dec_handler(extdef->child_list[0]);
        } else {
            // Function_Definition(函数定义)
            Panic_on(strcmp("Function_Definition", chd_name), "ExtDef");
            function_def_handler(extdef->child_list[0]);
        }
    }
}

void extDecList_handler(ASTNode *extdeclist, Type type) {
    if (extdeclist->child_num == 1) {
        // ExtDecList -> Vardec
        varDec_handler(extdeclist->child_list[0], type);
    } else {
        // ExtDecList -> Vardec COMMA ExtDecList
        Panic_on(extdeclist->child_num != 3, "ExtDecList");
        varDec_handler(extdeclist->child_list[0], type);
        extDecList_handler(extdeclist->child_list[2], type);
    }
}

void function_dec_handler(ASTNode *func_dec) {
    /**
     * 处理函数声明 Function_Declaration -> Specifier FunDec SEMI
     * （1）查表
     * （2）如果不为空，检查函数返回值，参数是否一致
     * （3）如果为空，插入符号
     */
    Panic_on(strcmp("Function_Declaration", func_dec->node_name), "Invalid Node");
    char *fname = func_dec->child_list[1]->child_list[0]->value.id;  // 函数名
    Log("FunName :%s", fname);

    Type returnType = specifier_handler(func_dec->child_list[0]);  // 函数返回值类型
    current_table = enter_scope(current_table);                    // 创建临时作用域将参数提取出来
    ParamList params = funDec_handler(func_dec->child_list[1]);    // 提取参数列表
    current_table = exit_scope(current_table);  // 由于是函数声明因此直接退出函数参数对应的作用域

    Symbol *fsym = lookup_symbol(current_table, fname, 1, FUNC);
    if (fsym != NULL) {
        // 说明此前进行过函数的声明或者定义，需要检查函数的返回值、参数列表是否一致
        if (!isEqualType(fsym->returnType, returnType) || !isEqualParamList(fsym->paramList, params)) {
            // 错误类型19：同名函数的返回值类型或者形参数量或者形参类型不一致
            sem_error(19, func_dec->lineno,
                      "The return value type or the number or type of formal parameters of a function with the same "
                      "name are inconsistent");
        }
    } else {
        // 说明此前尚未有过该函数的声明或者定义，需要插入表
        fsym = createFunSymbol(fname, current_table->scope, returnType, params, 0);
        add_symbol(current_table, fsym);
    }
}

void function_def_handler(ASTNode *func_def) {
    /**
     * 处理函数定义：Function_Definition -> Specifier FunDec CompSt
     */
    Type returnType = specifier_handler(func_def->child_list[0]);
    char *fname = func_def->child_list[1]->child_list[0]->value.id;  // 函数名
    Log("FunName :%s", fname);

    // 创建临时作用域将参数提取出来
    current_table = enter_scope(current_table);
    ParamList params = funDec_handler(func_def->child_list[1]);
    compst_handler(func_def->child_list[2], returnType);  //     相比于函数声明，在退出作用域前多加了Compst的处理
    current_table = exit_scope(current_table);

    Symbol *fsym = lookup_symbol(current_table, fname, 1, FUNC);
    if (fsym != NULL) {
        if (fsym->isFundef) {
            // 错误类型4：函数出现重复定义
            sem_error(4, func_def->lineno, "Duplicate definition of function");
            return;
        }
        // 说明此前进行过函数的声明，需要检查函数的返回值、参数列表是否一致
        if (!isEqualType(fsym->returnType, returnType) || !isEqualParamList(fsym->paramList, params)) {
            // 错误类型19：同名函数的返回值类型或者形参数量或者形参类型不一致
            sem_error(19, func_def->lineno,
                      "The return value type or the number or type of formal parameters of a function with the same "
                      "name are inconsistent");
        }
    } else {
        // 说明此前尚未有过该函数的声明或者定义，需要插入表
        fsym = createFunSymbol(fname, current_table->scope, returnType, params, 1);
        add_symbol(current_table, fsym);
    }
}

// ========================================================================== //
/* Specifiers */

/**
 * 给定一个Specifier节点，返回该节点的值类型（Type指针）
 * 这个Type指针可以用于跟在后面的变量的类型绑定
 */
Type specifier_handler(ASTNode *specifier) {
    ASTNode *child = specifier->child_list[0];
    if (!strcmp(child->node_name, "TYPE")) {  // Specifier -> TYPE
        return createBasicType(child->value.type);
    } else {  // Specifier -> StructSpecifier
        return struct_specifier_handler(child);
    }
}

Type struct_specifier_handler(ASTNode *str_specifier) {
    Type ret = NULL;
    if (str_specifier->child_num == 2) {
        // StructSpecifier -> STRUCT Tag ，Tag的子节点为ID
        // 这种情况算是结构体的使用, 比如 struct X a; 这里X为之前定义过的结构体的名字
        char *stru_name = str_specifier->child_list[1]->child_list[0]->value.id;
        Symbol *sym = lookup_symbol(current_table, stru_name, 1, VAR);
        if (sym == NULL) {
            sem_error(17, str_specifier->lineno, "Define variables directly using undefined structures.");
            return NULL;
        }
        ret = sym->vartype;
    } else {
        // StructSpecifier -> Struct OptTag LC DefList RC
        current_table = enter_scope(current_table);
        defList_handler(str_specifier->child_list[3]);
        FieldList fields = NULL;
        for (int i = 0; i < current_table->size; i++) {
            SymbolTableEntry *entry = current_table->table[i];
            while (entry != NULL) {
                Symbol *sym = entry->symbol;
                fields = create_FieldList(sym->name, sym->vartype, fields);
            }
        }
        ret = createStructureType(fields);
        current_table = exit_scope(current_table);
        Panic_on(current_table->scope != 0, "Struct Specifier");
        if (str_specifier->child_num == 5) {
            // 说明OptTag不为空，此时需要考虑加入符号表
            char *stru_name = str_specifier->child_list[1]->child_list[0]->value.id;
            Symbol *sym = lookup_symbol(current_table, stru_name, 1, VAR);
            if (sym) {
                // 错误类型16：结构体名字与前面定义过的结构体或者变量的名字重复
                sem_error(16, str_specifier->lineno,
                          "The structure name duplicates the name of a previously defined structure or variable");
            } else {
                sym = create_Symbol(stru_name, VAR, current_table->scope, ret);
                add_symbol(current_table, sym);
            }
        }
    }
    return ret;
}

// ========================================================================== //
/* Declarators */
void varDec_handler(ASTNode *vardec, Type type) {
    /**
     * 将变量与类型进行绑定,注意这里是变量的定义
     */
    Panic_on(strcmp("VarDec", vardec->node_name), "Vardec");
    if (vardec->child_num == 1) {
        // VarDec -> ID
        char *var_name = vardec->child_list[0]->value.id;
        Symbol *sym = lookup_symbol(current_table, var_name, 0, VAR);
        if (sym) {  // 变量重复定义
            sem_error(3, vardec->lineno, "Variable Duplicate Definition");
            return;
        }
        sym = (Symbol *)malloc(sizeof(Symbol));
        sym->vartype = VAR;
        sym->vartype = type;
        add_symbol(current_table, sym);
    } else {
        // VarDec -> VarDec LB INT RB
        Panic_on(vardec->child_num != 4);
        int arr_size = vardec->child_list[2]->value.ival;
        Type newtype = createArrayType(type, arr_size);
        varDec_handler(vardec->child_list[0], newtype);  // 这里非常巧妙地使用了递归调用，最后肯定会解析到VarDec->ID
    }
}

void varList_handler(ASTNode *varlist) {
    if (varlist->child_num == 1) {
        // VarList -> ParamDec
        paramDec_hanlder(varlist->child_list[0]);
    } else {
        // VarList -> ParamDec COMMA VarList
        Panic_on(varlist->child_num != 3, "VarList");
        paramDec_hanlder(varlist->child_list[0]);
        varList_handler(varlist->child_list[2]);
    }
}

void paramDec_hanlder(ASTNode *paramdec) {
    // ParamDec -> Specifier VarDec
    Panic_on(strcmp("ParamDec", paramdec->node_name), "ParamDec");
    Type *vartype = specifier_handler(paramdec->child_list[0]);
    varDec_handler(paramdec->child_list[1], vartype);
}

static int isEqualParamList(ParamList p1, ParamList p2) {
    // 如果两个指针本身相等，它们指向的内容也一定相等
    if (p1 == p2) {
        return 1;
    }
    // 如果其中一个指针为 NULL，它们不相等
    if (p1 == NULL || p2 == NULL) {
        return 0;
    }
    // 递归比较每一个参数的类型是否相等
    if (!isEqualType(p1->type, p2->type)) {
        return 0;
    }
    // 如果p1和p2都没有下一个参数，它们相等
    if (p1->next == NULL && p2->next == NULL) {
        return 1;
    }
    // 递归比较下一个参数
    return isEqualParamList(p1->next, p2->next);
}

ParamList funDec_handler(ASTNode *fundec) {
    /**
     * 提取出参数，返回ParamList
     *    FunDec -> ID LP VarList RP
     *           | ID LP RP
     */
    if (fundec->child_num == 4) {
        varList_handler(fundec->child_list[2]);
    }
    ParamList params = NULL;
    for (int i = 0; i < current_table->size; i++) {
        SymbolTableEntry *entry = current_table->table[i];
        while (entry != NULL) {
            Symbol *sym = entry->symbol;
            params = createParamList(sym->name, sym->vartype, params);  // 链表的插入
        }
    }
    return params;
}

// ========================================================================== //
/* Statements */

void compst_handler(ASTNode *compst, Type return_type) {
    // 在调用此函数之前，请确保已经调用了enter_scope()函数, 同样在处理完成之后，也需要调用者手动退出当前作用域
    // 这是因为有两种情况：
    // 函数定义中的CompSt要求能够感知到函数参数列表的内容，因此我们调用funDec_handler()会将作用域表填好一部分
    // Stmt中的CompSt并不要求能够感知到外部变量（进入此函数前currentTable表项为空）

    //  CompSt -> LC DefList StmtList RC
    defList_handler(compst->child_list[1]);
    stmtList_handler(compst->child_list[2]);
}

void stmtList_handler(ASTNode *stmtlist) {
    Panic_on(strcmp("StmtList", stmtlist->node_name), "StmtList");
    if (stmtlist->child_num == 1) {  // StmtList -> Stmt
        stmt_handler(stmtlist->child_list[0]);
    } else {  // StmtList -> Stmt StmtList
        stmt_handler(stmtlist->child_list[0]);
        stmtList_handler(stmtlist->child_list[1]);
    }
}
void stmt_handler(ASTNode *stmt) { Panic("TODO"); }

// ========================================================================== //
/* Local Definitions */

void defList_handler(ASTNode *deflist) {
    Panic_on(strcmp("DefList", deflist->node_name), "DefList");
    if (deflist->child_num == 1) {
        // DefList -> Def
        def_handler(deflist->child_list[0]);
    } else {
        // DefList -> Def DefList
        def_handler(deflist->child_list[0]);
        defList_handler(deflist->child_list[1]);
    }
}

void def_handler(ASTNode *def) {
    // Def -> Specifier DecList SEMI
    ASTNode *specifier = def->child_list[0];
    Type type = specifier_handler(specifier);
    decList_handler(def->child_list[1], type);
}

void decList_handler(ASTNode *declist, Type type) {
    if (declist->child_num == 1) {
        // DecList -> Dec
        dec_handler(declist->child_list[0], type);
    } else {
        // DecList -> Dec COMMA DecLsit
        dec_handler(declist->child_list[0], type);
        decList_handler(declist->child_list[2], type);
    }
}

void dec_handler(ASTNode *dec, Type type) {
    if (dec->child_num == 1) {
        // Dec -> VarDec
        varDec_handler(dec->child_list[0], type);
    } else {
        // Dec -> VarDec ASSIGNOP Exp
        // TODO: 待实现
    }
}

// ========================================================================== //
/* Expressions */
void exp_handler(ASTNode *exp) { Panic("TODO:"); }

void args_handler(ASTNode *args) { Panic("TODO:"); }

void sem_error(int type, int lineno, const char *mssg) {
    fprintf(stdout, "Error type %d at Line %d: %s.\n", type, lineno, mssg);
} /* errors reporting */
