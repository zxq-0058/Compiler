#include "semantics.h"

#include "logger.h"

static SymbolTable *current_table = NULL;
static SymbolTable *global_table = NULL;
static int struct_depth = 0;

// 判断两个Type是否一致
static int isEqualType(Type t1, Type t2) {
    if (t1 == NULL || t2 == NULL) {
        return 0;  // 如果其中一个指针为 NULL，它们不相等
    }
    if (t1 == t2) {
        return 1;  // 如果两个指针本身相等，它们指向的内容也一定相等
    }
    if (t1->kind != t2->kind) {
        return 0;  // 如果两个 Type 的 kind 不同，它们不相等
    }

    switch (t1->kind) {
        case BASIC:
            return t1->u.basic == t2->u.basic;  // 如果两个基本类型相等，它们相等
        case ARRAY:
            return isEqualType(t1->u.array.elem, t2->u.array.elem);  // 如果两个数组的元素类型和大小都相等，它们相等
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

    if (strcmp(basicTypeStr, "int") == 0) {
        newType->u.basic = INT;
    } else if (strcmp(basicTypeStr, "float") == 0) {
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

FieldList createFieldList(char *name, Type type, FieldList tail) {
    FieldList newField = (FieldList)malloc(sizeof(struct FieldList_));
    newField->name = name;
    newField->type = type;
    newField->tail = tail;
    return newField;
}

void insertFieldList(char *name, Type type, FieldList *old) {
    if (old == NULL) return;
    FieldList new_node = (FieldList)malloc(sizeof(struct FieldList_));
    new_node->name = name;
    new_node->type = type;
    new_node->tail = NULL;
    if (*old == NULL) {
        *old = new_node;
    } else {
        new_node->tail = *old;
        *old = new_node;
    }
}

Type find_field(Type struct_type, const char *field_name) {
    if (struct_type == NULL || struct_type->kind != STRUCTURE) {
        return NULL;  // 如果不是结构体或者指针为空，则返回NULL
    }

    FieldList fields = struct_type->u.structure;
    while (fields) {
        if (strcmp(fields->name, field_name) == 0) {
            return fields->type;  // 找到指定的域，返回它的类型
        }
        fields = fields->tail;  // 继续查找下一个域
    }

    return NULL;  // 如果没有找到指定的域，则返回NULL
}

void appendParamList(ParamList *paramList, char *name, Type type) {
    ParamList newParam = (ParamList)malloc(sizeof(struct ParamList_));
    newParam->name = name;
    newParam->type = type;
    newParam->next = NULL;

    if (*paramList == NULL) {
        *paramList = newParam;
    } else {
        ParamList currentParam = *paramList;
        while (currentParam->next != NULL) {
            currentParam = currentParam->next;
        }
        currentParam->next = newParam;
    }
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

void appendArgList(ArgList *arglist, Type type) {
    ArgList newArg = (ArgList)malloc(sizeof(struct ArgList_));
    newArg->type = type;
    newArg->next = NULL;

    if (*arglist == NULL) {
        *arglist = newArg;
    } else {
        ArgList currentArg = *arglist;
        while (currentArg->next != NULL) {
            currentArg = currentArg->next;
        }
        currentArg->next = newArg;
    }
}

int compareArgListParamList(ArgList argList, ParamList paramList) {
    // 比较参数列表的长度是否一致
    int argListLen = 0;
    ArgList argCurrent = argList;
    while (argCurrent != NULL) {
        argListLen++;
        argCurrent = argCurrent->next;
    }

    int paramListLen = 0;
    ParamList paramCurrent = paramList;
    while (paramCurrent != NULL) {
        paramListLen++;
        paramCurrent = paramCurrent->next;
    }

    if (argListLen != paramListLen) {
        return 0;
    }

    // 比较每个参数的类型是否一致
    argCurrent = argList;
    paramCurrent = paramList;
    while (argCurrent != NULL && paramCurrent != NULL) {
        if (!isEqualType(argCurrent->type, paramCurrent->type)) {
            return 0;
        }

        argCurrent = argCurrent->next;
        paramCurrent = paramCurrent->next;
    }

    return 1;
}

static inline Symbol *createVarSymbol(char *name, int scope, Type varType, int lineno, int isAssigned) {
    Symbol *varsym = (Symbol *)malloc(sizeof(Symbol));
    varsym->name = name;
    varsym->symType = VAR;
    varsym->scope = scope;
    varsym->vartype = varType;
    varsym->lineno = lineno;
    varsym->isAssigned = isAssigned;
    return varsym;
}

static inline Symbol *createFunSymbol(char *fname, int scope, Type returnType, ParamList params, int isFundef,
                                      int lineno) {
    Symbol *funsym = (Symbol *)malloc(sizeof(Symbol));
    funsym->name = fname;
    funsym->scope = scope;
    funsym->symType = FUNC;
    funsym->returnType = returnType;
    funsym->paramList = params;
    funsym->isFundef = isFundef;
    funsym->lineno = lineno;
    return funsym;
}

static inline Symbol *createStructureSymbol(char *name, int scope, Type structType, int lineno) {
    Symbol *structsym = (Symbol *)malloc(sizeof(Symbol));
    structsym->name = name;
    structsym->scope = scope;
    structsym->symType = STRUCT;
    structsym->structType = structType;
    structsym->lineno = lineno;
    return structsym;
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
    global_table = current_table;
}

// ========================================================================== //
/* High-level definitions */
void program_handler(ASTNode *program) {
    if (program == NULL) return;
    init();
    // Program -> ExtDefList
    extDefList_handler(program->child_list[0]);
    // 符号表插入完毕，检查函数是否定义
    Panic_on(current_table->scope != 0, "Invalid Scope!");
    for (int i = 0; i < current_table->size; i++) {
        SymbolTableEntry *entry = current_table->table[i];
        while (entry != NULL) {
            Symbol *sym = entry->symbol;
            if (sym->symType == FUNC && sym->isFundef == 0) {
                // 只有函数声明，没有函数定义
                sem_error(18, sym->lineno, "Undefined function \"%s\"", sym->name);
            }
            entry = entry->next;
        }
    }
}

void extDefList_handler(ASTNode *extdeflist) {
    Log("%s", extdeflist->node_name);
    if (extdeflist->child_num == 1) {
        // ExtDefList -> ExtDef
        extDef_handler(extdeflist->child_list[0]);
    } else {
        // ExtDefList -> ExtDef ExtDefList
        extDef_handler(extdeflist->child_list[0]);
        extDefList_handler(extdeflist->child_list[1]);
    }
}

void extDef_handler(ASTNode *extdef) {
    if (extdef->child_num == 3) {
        // Specifier ExtDecList SEMI(全局变量声明)
        // 每一个Specifier都会生成一个Type(基本类型或者是结构体类型)
        Type type = specifier_handler(extdef->child_list[0]);
        extDecList_handler(extdef->child_list[1], type);
    } else if (extdef->child_num == 2) {
        // Specifier SEMI(结构体变量声明)
        Type type = specifier_handler(extdef->child_list[0]);
    } else {
        const char *chd_name = extdef->child_list[0]->node_name;
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
        varDec_handler(extdeclist->child_list[0], type, 0);
    } else {
        // ExtDecList -> Vardec COMMA ExtDecList
        Panic_on(extdeclist->child_num != 3, "ExtDecList");
        varDec_handler(extdeclist->child_list[0], type, 0);
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
        fsym = createFunSymbol(fname, global_table->scope, returnType, params, 0, func_dec->lineno);
        add_symbol(global_table, fsym);
    }
}

void function_def_handler(ASTNode *func_def) {
    /**
     * 处理函数定义：Function_Definition -> Specifier FunDec CompSt
     */
    Type returnType = specifier_handler(func_def->child_list[0]);    // 函数返回值类型
    char *fname = func_def->child_list[1]->child_list[0]->value.id;  // 函数名
    Log("FunName :%s", fname);

    // 创建临时作用域将参数提取出来
    current_table = enter_scope(current_table);
    // Fundec的产生式为：FunDec -> ID LP VarList RP | ID LP RP 比如: int foo(int a, int b) 或者 int foo()
    ParamList params = funDec_handler(func_def->child_list[1]);
    Symbol *fsym = lookup_symbol(current_table, fname, 1, FUNC);
    if (fsym != NULL) {
        if (fsym->isFundef) {
            // 错误类型4：函数出现重复定义
            sem_error(4, func_def->lineno, "Duplicate definition of function");
        }
        // 说明此前进行过函数的声明，需要检查函数的返回值、参数列表是否一致
        else if (!isEqualType(fsym->returnType, returnType) || !isEqualParamList(fsym->paramList, params)) {
            // 错误类型19：同名函数的返回值类型或者形参数量或者形参类型不一致
            sem_error(19, func_def->lineno,
                      "The return value type or the number or type of formal parameters of a function with the same "
                      "name are inconsistent");
        } else {
            // 如果函数定义没有冲突
            fsym->isFundef = 1;
        }
    } else {
        // 说明此前尚未有过该函数的声明或者定义，需要插入表
        fsym = createFunSymbol(fname, global_table->scope, returnType, params, 1, func_def->lineno);
        add_symbol(global_table, fsym);
    }
    compst_handler(func_def->child_list[2], returnType);  //     相比于函数声明，在退出作用域前多加了Compst的处理
    current_table = exit_scope(current_table);
    Panic_on(current_table->scope != 0, "Invalid Scope");
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
        return structSpecifier_handler(child);
    }
}

Type structSpecifier_handler(ASTNode *str_specifier) {
    Type ret = NULL;
    if (str_specifier->child_num == 2) {
        // StructSpecifier -> STRUCT Tag ，Tag的子节点为ID
        // 这种情况算是结构体的使用, 比如 struct X a; 这里X为之前定义过的结构体的名字
        char *stru_name = str_specifier->child_list[1]->child_list[0]->value.id;
        Symbol *sym = lookup_symbol(global_table, stru_name, 1, STRUCT);
        if (sym == NULL) {
            sem_error(17, str_specifier->lineno, "Define variables directly using undefined structures");
            return NULL;
        }
        ret = sym->structType;
    } else {
        // StructSpecifier -> Struct OptTag LC DefList RC
        current_table = enter_scope(current_table);
        struct_depth++;  // 全局变量表示当前正在分析结构体，防止与错误类型3冲突
        ret = createStructureType(NULL);  // 创建结构体类型(处理之前的域为空)
        for (int i = 0; i < str_specifier->child_num; i++) {
            if (strcmp(str_specifier->child_list[i]->node_name, "DefList") == 0) {
                defList_handler(str_specifier->child_list[i],
                                &ret->u.structure);  // 处理结构体的域，由于是引用传递，所以fields会被修改
            }
        }

        struct_depth--;
        Panic_on(struct_depth < 0, "struct_depth error");
        // Panic_on(current_table->scope != 0, "Struct Specifier");
        if (strcmp(str_specifier->child_list[1]->node_name, "OptTag") == 0) {
            // 说明OptTag不为空，此时需要考虑加入符号表
            char *stru_name = str_specifier->child_list[1]->child_list[0]->value.id;
            Symbol *sym = lookup_symbol(global_table, stru_name, 1, STRUCT);
            Symbol *sym2 = lookup_symbol(global_table, stru_name, 1, VAR);
            current_table = exit_scope(current_table);  // 退出临时作用域
            if (sym || sym2) {
                // 错误类型16：结构体名字与前面定义过的结构体或者变量的名字重复
                sem_error(16, str_specifier->lineno,
                          "The structure \"%s\" duplicates the name of a previously defined structure or variable",
                          stru_name);
            } else {
                if (sym == NULL)
                    sym = createStructureSymbol(stru_name, global_table->scope, ret, str_specifier->lineno);
                add_symbol(global_table, sym);
            }
        } else {
            current_table = exit_scope(current_table);  // 退出临时作用域
        }
    }
    return ret;
}

// ========================================================================== //
/* Declarators */

// 加入符号表，同时返回变量对应的symbol
Symbol *varDec_handler(ASTNode *vardec, Type type, int isAssigned) {
    /**
     * 将变量与类型进行绑定,注意这里是变量的定义, isAssigned表示后面有没有跟Assign操作（赋初值）
     */
    Panic_on(strcmp("VarDec", vardec->node_name), "Vardec");
    if (vardec->child_num == 1) {
        // VarDec -> ID
        char *var_name = vardec->child_list[0]->value.id;
        // 先在全局的表里面查找是否有同名的结构体
        Symbol *sym_stru = lookup_symbol(global_table, var_name, 1, STRUCT);
        if (sym_stru) {
            // 错误类型3：变量名与前面定义过的结构体重复
            sem_error(3, vardec->lineno, "The variable \"%s\" duplicates the name of a previously defined structure",
                      sym_stru->name);
        }

        Symbol *sym = lookup_symbol(current_table, var_name, 0, VAR);
        if (sym && !struct_depth) {  // 变量重复定义
            sem_error(3, vardec->lineno, "Redefined Variable \"%s\"", sym->name);
            return sym;
        } else if (sym && struct_depth) {  // 结构体中的域重复定义
            sem_error(15, vardec->lineno, "Redefined field \"%s\" in the structure", sym->name);
            return sym;
        }
        sym = createVarSymbol(var_name, current_table->scope, type, vardec->lineno, isAssigned);
        add_symbol(current_table, sym);
        if (isAssigned && struct_depth) {
            // 结构体中的域定义时赋初值
            sem_error(15, vardec->lineno, "The field \"%s\" in the structure is initialized", sym->name);
        }
        return sym;
    } else {
        // VarDec -> VarDec LB INT RB
        Panic_on(vardec->child_num != 4, "VarDec");
        int arr_size = vardec->child_list[2]->value.ival;  // 注意这里是从右往左解析
        Type newtype = createArrayType(type, arr_size);
        return varDec_handler(vardec->child_list[0], newtype,
                              isAssigned);  // 这里非常巧妙地使用了递归调用，最后肯定会解析到VarDec->ID
    }
}

// 通过调用ParamDec_handler()解析参数，注意到VarList只出现在FunDec产生式中
void varList_handler(ASTNode *varlist, ParamList *params) {
    if (varlist->child_num == 1) {
        // VarList -> ParamDec
        Symbol *sym = paramDec_hanlder(varlist->child_list[0]);  // 获得某个形式参数的symbol
        appendParamList(params, sym->name, sym->vartype);        // 将形式参数append到参数列表中
    } else {
        // VarList -> ParamDec COMMA VarList
        Panic_on(varlist->child_num != 3, "VarList");
        Symbol *sym = paramDec_hanlder(varlist->child_list[0]);  // 获得某个形式参数的symbol
        appendParamList(params, sym->name, sym->vartype);        // 将形式参数append到参数列表中
        varList_handler(varlist->child_list[2], params);
    }
}

Symbol *paramDec_hanlder(ASTNode *paramdec) {
    // ParamDec -> Specifier VarDec
    Panic_on(strcmp("ParamDec", paramdec->node_name), "ParamDec");
    Type vartype = specifier_handler(paramdec->child_list[0]);
    return varDec_handler(paramdec->child_list[1], vartype, 0);
}

ParamList funDec_handler(ASTNode *fundec) {
    /**
     * 提取出参数，返回ParamList(注意保持有序性)
     *    FunDec -> ID LP VarList RP
     *           | ID LP RP
     */
    ParamList params = NULL;  // 不太确定
    if (fundec->child_num == 4) {
        varList_handler(fundec->child_list[2], &params);
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
    Panic_ON(strcmp("CompSt", compst->node_name), "CompSt");
    //  CompSt -> LC DefList StmtList RC
    // 注意到这里DefList和StmtList任意一个都可能是空节点
    for (int i = 0; i < compst->child_num; i++) {
        const char *chname = compst->child_list[i]->node_name;
        if (strcmp(chname, "DefList") == 0)
            defList_handler(compst->child_list[i], NULL);
        else if (strcmp(chname, "StmtList") == 0)
            stmtList_handler(compst->child_list[i], return_type);
    }
}

void stmtList_handler(ASTNode *stmtlist, Type return_type) {
    Panic_on(strcmp("StmtList", stmtlist->node_name), "StmtList");
    if (stmtlist->child_num == 1) {  // StmtList -> Stmt
        stmt_handler(stmtlist->child_list[0], return_type);
    } else {  // StmtList -> Stmt StmtList
        stmt_handler(stmtlist->child_list[0], return_type);
        stmtList_handler(stmtlist->child_list[1], return_type);
    }
}
void stmt_handler(ASTNode *stmt, Type return_type) {
    Panic_on(strcmp("Stmt", stmt->node_name), "Stmt");
    if (stmt->child_num == 1) {
        // Stmt -> CompSt
        current_table = enter_scope(current_table);
        compst_handler(stmt->child_list[0], return_type);
        current_table = exit_scope(current_table);
    } else if (stmt->child_num == 2) {
        // Stmt -> Exp SEMI
        exp_handler(stmt->child_list[0]);
    } else if (stmt->child_num == 3) {
        // Stmt -> RETURN Exp SEMI
        ExprRet r = exp_handler(stmt->child_list[1]);
        if (!isEqualType(return_type, r.expType)) {
            sem_error(8, stmt->lineno, "Type mismatched for return");
        }
    } else if (stmt->child_num == 5) {
        if (strcmp("IF", stmt->child_list[0]->node_name)) {
            // Stmt ->  IF LP Exp RP Stmt
            ExprRet r = exp_handler(stmt->child_list[2]);
            if (r.expType == NULL || r.expType->kind != BASIC || r.expType->u.basic != INT) {
                sem_error(7, stmt->lineno, "Type mismatched for IF");
            }
            stmt_handler(stmt->child_list[4], return_type);
        } else {
            // Stmt -> WHILE LP Exp RP Stmt
            ExprRet r = exp_handler(stmt->child_list[2]);
            if (r.expType == NULL || r.expType->kind != BASIC || r.expType->u.basic != INT) {
                sem_error(7, stmt->lineno, "Type mismatched for WHILE");
            }
            stmt_handler(stmt->child_list[4], return_type);
        }
    } else {
        // Stmt -> IF LP Exp RP Stmt ELSE Stmt
        ExprRet r = exp_handler(stmt->child_list[2]);
        if (r.expType == NULL || r.expType->kind != BASIC || r.expType->u.basic != INT) {
            sem_error(7, stmt->lineno, "Type mismatched for IF");
        }
        stmt_handler(stmt->child_list[4], return_type);
        stmt_handler(stmt->child_list[6], return_type);
    }
}

// ========================================================================== //
/* Local Definitions */

void defList_handler(ASTNode *deflist, FieldList *fields) {
    Panic_on(strcmp("DefList", deflist->node_name), "DefList");
    if (deflist->child_num == 1) {
        // DefList -> Def
        def_handler(deflist->child_list[0], fields);
    } else {
        // DefList -> Def DefList
        def_handler(deflist->child_list[0], fields);
        defList_handler(deflist->child_list[1], fields);
    }
}

void def_handler(ASTNode *def, FieldList *fields) {
    // Def -> Specifier DecList SEMI
    Panic_on(strcmp("Def", def->node_name), "Def");
    ASTNode *specifier = def->child_list[0];
    Type type = specifier_handler(specifier);
    decList_handler(def->child_list[1], type, fields);
}

void decList_handler(ASTNode *declist, Type type, FieldList *fields) {
    Panic_on(strcmp("DecList", declist->node_name), "DecList");
    if (declist->child_num == 1) {
        // DecList -> Dec
        Symbol *sym = dec_handler(declist->child_list[0], type);
        insertFieldList(sym->name, sym->vartype, fields);  // 这里的fields是一个指针，因此可以直接修改
    } else {
        // DecList -> Dec COMMA DecLsit
        Symbol *sym = dec_handler(declist->child_list[0], type);
        insertFieldList(sym->name, sym->vartype, fields);  // 这里的fields已经被修改了
        decList_handler(declist->child_list[2], type, fields);
    }
}

Symbol *dec_handler(ASTNode *dec, Type type) {
    Panic_on(strcmp("Dec", dec->node_name), "Dec");
    if (dec->child_num == 1) {
        // Dec -> VarDec
        return varDec_handler(dec->child_list[0], type, 0);
    } else {
        // Dec -> VarDec ASSIGNOP Exp
        ExprRet ret = exp_handler(dec->child_list[2]);
        Symbol *sym = varDec_handler(dec->child_list[0], type, 1);
        if (!isEqualType(sym->vartype, ret.expType)) {
            // 注意不能使用type，而是应该使用sym->vartype，这是因为由上层函数传递的type只是specifier的类型,在数组的情况下，type是不完整的
            sem_error(5, dec->lineno, "Type mismatched for assignment");
        }
        return sym;
    }
}

// ========================================================================== //

/* Expressions */
// 每一个表达式都有两种属性，一种是表达式的类型，一种是表达式是否只有右值
ExprRet exp_handler(ASTNode *exp) {
    /**
     * 返回值为Int,如果为0表示只有右值的表达式
     * expType需要返回调用者，以便告诉上层调用者当前表达式的Type
     */
    ExprRet ret = {-1, NULL};
    switch (exp->exp_type) {
        case ASSIGN_EXP: {
            // Exp -> Exp ASSIGNOP Exp
            ExprRet r1 = exp_handler(exp->child_list[0]);
            ExprRet r2 = exp_handler(exp->child_list[2]);
            if (r1.expType != NULL && !isEqualType(r1.expType, r2.expType)) {
                sem_error(5, exp->lineno, "Type mismatched for assignment");
            }
            if (r1.onlyRight) {
                sem_error(6, exp->lineno, "The left-hand side of an assignment must be a variable");
            }
            ret.expType = r1.expType;
            ret.onlyRight = 0;
            break;
        }
        case BINARY_EXP: {
            // Exp -> Exp AND/OR/RELOP/etc. Exp
            ExprRet r1 = exp_handler(exp->child_list[0]);
            ExprRet r2 = exp_handler(exp->child_list[2]);
            if (!isEqualType(r1.expType, r2.expType) || r1.expType->kind != BASIC) {
                // 左右两边的操作数类型不匹配；或者类型不是基本类型
                sem_error(7, exp->lineno, "Type mismatched for operands");
            }
            ret.expType = r1.expType;
            ret.onlyRight = 1;
            break;
        }
        case P_EXP: {  // LP EXP RP，ret和Type都取决于中间的Exp
            ret = exp_handler(exp->child_list[1]);
            break;
        }
        case UNARY_EXP: {  // MINUS/NOT EXP
            ret = exp_handler(exp->child_list[1]);
            ret.onlyRight = 1;
            if (ret.expType == NULL || ret.expType->kind != BASIC) {
                // 类型不是基本类型
                sem_error(7, exp->lineno, "Type mismatched for operands");
            }
            break;
        }
        case FUN_EXP: {
            // 函数调用,检查参数类型是否匹配
            char *func_name = exp->child_list[0]->value.id;
            Symbol *sym1 = lookup_symbol(current_table, func_name, 1, VAR);
            if (sym1) {
                sem_error(11, exp->lineno, "\"%s\" is not a function", func_name);
            } else {
                Symbol *sym2 = lookup_symbol(current_table, func_name, 1, FUNC);
                if (!sym2) {
                    // 错误类型2：函数在调用时未经定义
                    sem_error(2, exp->lineno, "Undefined function \"%s\"", func_name);
                } else {
                    ArgList args = NULL;
                    if (exp->child_num == 4) {
                        // 说明有参数需要提取 : Exp -> ID LP Args RP
                        args_handler(exp->child_list[2], &args);
                    }
                    if (!compareArgListParamList(args, sym2->paramList)) {
                        sem_error(9, exp->lineno, "Function \"%s\" is not applicable for arguments", func_name);
                    }
                    ret.expType = sym2->returnType;
                    ret.onlyRight = 1;
                }
            }
            break;
        }
        case ARR_EXP: {
            ExprRet r1 = exp_handler(exp->child_list[0]);
            ExprRet r2 = exp_handler(exp->child_list[2]);
            if (r1.expType == NULL || r1.expType->kind != ARRAY) {
                // 错误类型10：对非数组行变量不能使用[...]访问
                sem_error(10, exp->lineno, "Cannot use array indexing with a non-array type variable");
            } else {
                // 如果是数组类型，记得取出elem的类型
                ret.expType = r1.expType->u.array.elem;
            }
            if (r2.expType == NULL || r2.expType->kind != BASIC || r2.expType->u.basic != INT) {
                // 错误类型12: 数组访问操作符[...]中出现非整数
                sem_error(12, exp->lineno, "Array index must be an integer");
            }
            ret.onlyRight = 0;
            break;
        }
        case STRU_EXP: {
            const char *filed_name = exp->child_list[2]->value.id;
            ExprRet r = exp_handler(exp->child_list[0]);
            if (r.expType == NULL || r.expType->kind != STRUCTURE) {
                sem_error(13, exp->lineno, "Illegal use of \".\"");
            } else {
                ret.expType = find_field(r.expType, filed_name);
                if (ret.expType == NULL) sem_error(14, exp->lineno, "Non-existent field \"n\"");
            }
            ret.onlyRight = 0;
            break;
        }
        case ID_EXP: {
            char *id_name = exp->child_list[0]->value.id;
            Symbol *sym = lookup_symbol(current_table, id_name, 1, VAR);
            if (!sym) {
                sem_error(1, exp->lineno, "Undefined variable \"%s\"", id_name);
            } else {
                ret.expType = sym->vartype;
            }
            ret.onlyRight = 0;
            break;
        }
        case INT_EXP: {
            ret.expType = createBasicType("int");
            ret.onlyRight = 1;
            break;
        }
        case FLOAT_EXP: {
            ret.expType = createBasicType("float");
            ret.onlyRight = 1;
            break;
        }
        default:
            Panic("Invalid Exp Type!");
    }
    return ret;
}

void args_handler(ASTNode *args, ArgList *arglist) {
    if (args->child_num == 1) {
        // Args -> Exp
        ExprRet ret = exp_handler(args->child_list[0]);
        appendArgList(arglist, ret.expType);
    } else {
        // Args -> Exp COMMA Args
        Panic_on(args->child_num != 3, "Args");
        ExprRet ret = exp_handler(args->child_list[0]);
        appendArgList(arglist, ret.expType);
        args_handler(args->child_list[2], arglist);
    }
}

void sem_error(int type, int lineno, const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stdout, "Error type %d at Line %d: ", type, lineno);
    vfprintf(stdout, format, args);
    fprintf(stdout, ".\n");
    va_end(args);
}
