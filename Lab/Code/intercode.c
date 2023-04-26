#include "intercode.h"

// Path: Code/intercode.c

int calculateMemSize(Type t);
int get_offset_in_struct(Type t, char *name);
static inline Operand new_op(Operand src);
static inline Operand new_label();
static inline Operand new_var_op();
static inline Operand new_tmp_op();
static inline Operand new_const(int value);
static inline Operand new_func_op(char *fname);
static inline Operand new_addr_op();
static inline void dump_op(Operand dest, Operand src);
void insert_code(InterCodes code);
void insert_label_ir(Operand label);
void insert_func_ir(Operand func);
void insert_assign_ir(int assign_type, Operand left, Operand right);
void insert_param_ir(Operand param);
void insert_ret_ir(Operand ret);
void insert_binop_ir(int kind, Operand result, Operand op1, Operand op2);
void insert_relop_ir(char *relop, Operand x, Operand y, Operand label);
void insert_goto_ir(Operand label);
void insert_read_ir(Operand op);
void insert_write_ir(Operand op);
void insert_call_ir(Operand result, Operand func);
void insert_arg_ir(Operand arg);
void insert_dec_ir(Operand op, int size);
void translate_program(ASTNode *program, FILE *f);
void translate_extdeflist(ASTNode *extdeflist);
void translate_extdef(ASTNode *extdef);
void translate_extdeclist(ASTNode *extdeclist);
Type translate_specifier(ASTNode *specifier);
Type translate_structspecifier(ASTNode *str_specifier);
void translate_fundec(ASTNode *fundec, Type retType);
void translate_varlist(ASTNode *varlist);
void translate_paramdec(ASTNode *paramdec);
void translate_compst(ASTNode *compst, Type return_type);
void translate_stmtlist(ASTNode *stmtlist, Type ret_type);
void translate_cond(ASTNode *exp, Operand label_true, Operand label_false);
void translate_stmt(ASTNode *stmt, Type ret_type);
void translate_deflist(ASTNode *deflist, FieldList *fields);
void translate_def(ASTNode *def, FieldList *fields);
void translate_declist(ASTNode *declist, Type type, FieldList *fields);
void translate_dec(ASTNode *dec, Type type);
Type translate_exp(ASTNode *exp, Operand place);
void exp_assign(ASTNode *exp, Operand place);
Type exp_struct(ASTNode *exp, Operand place);
Type exp_array(ASTNode *exp, Operand place);
void exp_binary(ASTNode *exp, Operand place);
void translate_args(ASTNode *args);
void print_intercodes(FILE *fp);

//============================================================================================================================//

extern SymbolTable *current_table;
extern SymbolTable *global_table;
static int struct_depth = 0;  // 判断结构体嵌套层数

/// @brief 给定一个类型，返回其内存大小，这一步在将符号插入符号表时会完成
/// @note 基本类型占用4个字节，数组类型占用elem->memSize * size个字节，结构体类型占用所有域的大小之和
int calculateMemSize(Type t) {
    if (t->memSize != 0) return t->memSize;
    switch (t->kind) {
        case BASIC:
            return sizeof(int);  // 假设基本类型都占用4个字节
        case ARRAY:
            return calculateMemSize(t->u.array.elem) * t->u.array.size;
        case STRUCTURE: {
            int size = 0;
            FieldList f = t->u.structure;
            while (f) {
                size += calculateMemSize(f->type);
                f = f->tail;
            }
            return size;
        }
    }
    Panic("Unknown type kind");
}

/// @brief 给定一个结构体类型和需要查找的域名，返回其域的偏移量
int get_offset_in_struct(Type t, char *name) {
    Panic_on(!t || t->kind != STRUCTURE, "Not a structure type");
    FieldList f = t->u.structure;
    int offset = 0;
    while (f) {
        if (strcmp(f->name, name) == 0) {
            return offset;
        }
        offset += calculateMemSize(f->type);
        f = f->tail;
    }
    Panic("Unknown field name");
}

//============================================================================================================================//

//============================================================================================================================//
//------------------------------------------------ Operand相关部分
//------------------------------------------------------------//

static int label_count = 1;  // label的计数器
static int var_count = 1;    // 变量的计数器
static int tmp_count = 1;    // 临时变量计数器

/// @brief 返回一个新的操作数（kind等信息拷贝op），如果op == NULL，则申请一个未初始化的Operand
static inline Operand new_op(Operand src) {
    Operand op = (Operand)malloc(sizeof(struct Operand_));
    if (src) memcpy(op, src, sizeof(struct Operand_));
    return op;
}

/// @brief 返回一个新的label, label_coun++
/// @return Operand
static inline Operand new_label() {
    Operand label = (Operand)malloc(sizeof(struct Operand_));
    label->kind = IR_LABEL;
    label->u.var_no = label_count++;
    return label;
}

/// @brief 返回一个新的变量，var_count++
/// @return Operand
static inline Operand new_var_op() {
    Operand var = (Operand)malloc(sizeof(struct Operand_));
    var->kind = OP_VARIABLE;
    var->u.var_no = var_count++;
    return var;
}

/// @brief 返回一个新的临时变量, tmp_count++，默认tmp_addr = 0，即默认临时变量存值而非地址
static inline Operand new_tmp_op() {
    Operand temp = (Operand)malloc(sizeof(struct Operand_));
    temp->kind = OP_TEMP;
    temp->u.tmp.tmp_no = tmp_count++;
    temp->u.tmp.tmp_addr = 0;
    return temp;
}

// 限定整数常量
static inline Operand new_const(int value) {
    Operand constant = (Operand)malloc(sizeof(struct Operand_));
    constant->kind = OP_CONSTANT;
    constant->u.value = value;
    return constant;
}

/// @brief  返回一个函数操作数
static inline Operand new_func_op(char *fname) {
    Operand func = (Operand)malloc(sizeof(struct Operand_));
    func->kind = OP_FUNCTION;
    func->u.func_name = fname;
    return func;
}

/// @brief 返回一个地址操作数，格式为v_{var_count}，对应函数参数为数组或者是结构体的情况
/// @note 注意不是临时变量存地址的情况，临时变量存放地址对应OP_TEMP并且tmp_addr = 1的情况
static inline Operand new_addr_op() {
    Operand addr = (Operand)malloc(sizeof(struct Operand_));
    addr->kind = OP_ADDRESS;
    addr->u.var_no = var_count++;
    return addr;
}

/// @brief 将src的内容复制到dest
static inline void dump_op(Operand dest, Operand src) {
    if (dest == NULL || src == NULL) return;
    dest->kind = src->kind;
    dest->u = src->u;
}

//============================================================================================================================//
//---------------------------------- Intercode 和 IR(指令) 相关部分 --------------------------------------------//

InterCodes head = NULL, tail = NULL;

// 链表插入
void insert_code(InterCodes code) {
    if (head == NULL) {
        head = code;
        tail = code;
        code->next = NULL;
        code->prev = NULL;
    } else {
        tail->next = code;
        code->prev = tail;
        tail = code;
        code->next = NULL;
    }
}

/// @brief LABEL label
void insert_label_ir(Operand label) {
    InterCodes code = (InterCodes)malloc(sizeof(struct InterCodes_));
    code->code = (InterCode)malloc(sizeof(struct InterCode_));
    code->code->kind = IR_LABEL;
    code->code->u.one.op = label;
    insert_code(code);
}

/// @brief FUNCTION func
void insert_func_ir(Operand func) {
    InterCodes code = (InterCodes)malloc(sizeof(struct InterCodes_));
    code->code = (InterCode)malloc(sizeof(struct InterCode_));
    code->code->kind = IR_FUNCTION;
    code->code->u.one.op = func;
    insert_code(code);
}

// x := y， 中间代码中的赋值操作
void insert_assign_ir(int assign_type, Operand left, Operand right) {
    InterCodes code = (InterCodes)malloc(sizeof(struct InterCodes_));
    code->code = (InterCode)malloc(sizeof(struct InterCode_));
    code->code->kind = IR_ASSIGN;
    code->code->u.assign.ass_type = assign_type;
    code->code->u.assign.left = left;
    code->code->u.assign.right = right;
    insert_code(code);
}

/// @brief PARAM x
/// @param param x
void insert_param_ir(Operand param) {
    InterCodes code = (InterCodes)malloc(sizeof(struct InterCodes_));
    code->code = (InterCode)malloc(sizeof(struct InterCode_));
    code->code->kind = IR_PARAM;
    code->code->u.one.op = param;
    insert_code(code);
}

/// @brief RETURN x
/// @param ret x
void insert_ret_ir(Operand ret) {
    InterCodes code = (InterCodes)malloc(sizeof(struct InterCodes_));
    code->code = (InterCode)malloc(sizeof(struct InterCode_));
    code->code->kind = IR_RETURN;
    code->code->u.one.op = ret;
    insert_code(code);
}

/// @brief  生成二元运算的的中间代码（三个操作数），比如 x := y + z
/// @param kind 合法的类型： ADD, SUB, MUL, DIV, ADDRADD(地址加: x = &y + z)
/// @param result 结果
/// @param op1  操作数1
/// @param op2  操作数2
void insert_binop_ir(int kind, Operand result, Operand op1, Operand op2) {
    // result := op1 kind op2
    InterCodes code = (InterCodes)malloc(sizeof(struct InterCodes_));
    code->code = (InterCode)malloc(sizeof(struct InterCode_));
    code->code->kind = kind;
    code->code->u.binop.result = result;
    code->code->u.binop.op1 = op1;
    code->code->u.binop.op2 = op2;
    insert_code(code);
}

/// @brief IF x relop y GOTO label
void insert_relop_ir(char *relop, Operand x, Operand y, Operand label) {
    InterCodes code = (InterCodes)malloc(sizeof(struct InterCodes_));
    code->code = (InterCode)malloc(sizeof(struct InterCode_));
    code->code->kind = IR_IFGOTO;
    code->code->u.ifgoto.x = x;
    code->code->u.ifgoto.y = y;
    code->code->u.ifgoto.relop = relop;
    code->code->u.ifgoto.z = label;
    insert_code(code);
}

/// @brief GOTO label
void insert_goto_ir(Operand label) {
    InterCodes code = (InterCodes)malloc(sizeof(struct InterCodes_));
    code->code = (InterCode)malloc(sizeof(struct InterCode_));
    code->code->kind = IR_GOTO;
    code->code->u.one.op = label;
    insert_code(code);
}

/// @brief READ x
/// @param op x
void insert_read_ir(Operand op) {
    InterCodes code = (InterCodes)malloc(sizeof(struct InterCodes_));
    code->code = (InterCode)malloc(sizeof(struct InterCode_));
    code->code->kind = IR_READ;
    code->code->u.one.op = op;
    insert_code(code);
}

/// @brief WRITE x
/// @param op x
void insert_write_ir(Operand op) {
    InterCodes code = (InterCodes)malloc(sizeof(struct InterCodes_));
    code->code = (InterCode)malloc(sizeof(struct InterCode_));
    code->code->kind = IR_WRITE;
    code->code->u.one.op = op;
    insert_code(code);
}

/// @brief x := CALL func
/// @param result x
/// @param func func
void insert_call_ir(Operand result, Operand func) {
    InterCodes code = (InterCodes)malloc(sizeof(struct InterCodes_));
    code->code = (InterCode)malloc(sizeof(struct InterCode_));
    code->code->kind = IR_CALL;
    code->code->u.two.left = result;
    code->code->u.two.right = func;
    insert_code(code);
}

/// @brief ARG x
/// @param arg x
void insert_arg_ir(Operand arg) {
    InterCodes code = (InterCodes)malloc(sizeof(struct InterCodes_));
    code->code = (InterCode)malloc(sizeof(struct InterCode_));
    code->code->kind = IR_ARG;
    code->code->u.one.op = arg;
    insert_code(code);
}

/// @brief DEC x size
/// @param op
/// @param size
void insert_dec_ir(Operand op, int size) {
    InterCodes code = (InterCodes)malloc(sizeof(struct InterCodes_));
    code->code = (InterCode)malloc(sizeof(struct InterCode_));
    code->code->kind = IR_DEC;
    code->code->u.dec.op = op;
    code->code->u.dec.size = size;
    insert_code(code);
}

//============================================================================================================================//
//---------------------------------- 语法树遍历部分 Translate_X() --------------------------------------------//

/// @brief 生成中间代码并输出到 f 文件
/// @param program AST树的根节点
/// @param f 文件指针
void translate_program(ASTNode *program, FILE *f) {
    Panic_ON(current_table != global_table, "current_table != global_table");
    translate_extdeflist(program->child_list[0]);
    print_intercodes(f);
}

void translate_extdeflist(ASTNode *extdeflist) {
    if (match(extdeflist, 3, "ExtDefList", "ExtDef", "ExtDefList")) {
        translate_extdef(extdeflist->child_list[0]);
        translate_extdeflist(extdeflist->child_list[1]);
    } else {
        translate_extdef(extdeflist->child_list[0]);
    }
}

void translate_extdef(ASTNode *extdef) {
    if (match(extdef, 3, "ExtDef", "Specifier", "ExtDecList", "SEMI")) {
        // 全局变量
        translate_specifier(extdef->child_list[0]);
        translate_extdeclist(extdef->child_list[1]);
    } else if (match(extdef, 3, "ExtDef", "Specifier", "SEMI")) {
        // 全局结构体
        translate_specifier(extdef->child_list[0]);
    } else if (match(extdef, 2, "ExtDef", "Function_Definition")) {
        // 函数定义
        Type retType = translate_specifier(extdef->child_list[0]->child_list[0]);
        current_table = enter_scope(current_table);  // 进入函数体之前依旧需要创建新的作用域
        translate_fundec(extdef->child_list[0]->child_list[1], retType);
        translate_compst(extdef->child_list[0]->child_list[2], retType);
        current_table = exit_scope(current_table);
    } else {
        // 不允许函数声明
        Panic("Invalid ExtDef");
    }
}

void translate_extdeclist(ASTNode *extdeclist) { Panic("Should not reach here since global var is not allowed!"); }

Type translate_specifier(ASTNode *specifier) {
    ASTNode *child = specifier->child_list[0];
    if (match(specifier, 2, "Specifier", "TYPE")) {
        return createBasicType(child->value.type);
    } else {
        return translate_structspecifier(child);
    }
}

/// @brief 返回结构体类型，当遇到结构体定义时，会通过查全局表获得Type，然后借助calculateMemSize计算得到内存大小
/// `sym->structType->memSize = calculateMemSize(sym->structType);`
/// @return sym->structType
Type translate_structspecifier(ASTNode *str_specifier) {
    if (match(str_specifier, 3, "StructSpecifier", "STRUCT", "Tag")) {
        char *stru_name = str_specifier->child_list[1]->child_list[0]->value.id;
        Symbol *sym = lookup_symbol(global_table, stru_name, 0, STRUCT);
        Panic_on(sym == NULL, "Struct %s not defined", stru_name);
        return sym->structType;
    } else {
        // StructSpecifier -> STRUCT OptTag LC DefList RC
        // 我做了一个不太好的假设：不会出现匿名结构体的定义
        char *stru_name = str_specifier->child_list[1]->child_list[0]->value.id;
        struct_depth++;
        Symbol *sym = lookup_symbol(global_table, stru_name, 0, STRUCT);
        Panic_on(sym == NULL, "Struct %s not defined", stru_name);
        if (strcmp(str_specifier->child_list[3]->node_name, "DefList") == 0) {
            translate_deflist(str_specifier->child_list[3], NULL);
        }
        sym->structType->memSize = calculateMemSize(sym->structType);
        struct_depth--;
        return sym->structType;
    }
}

/// @brief
/// 在实验三中，每一个Symbol都需要与一个Operand绑定，但是由于Symbol可以绑定OP_VAR、OP_ADDR类型，因此具体的绑定操作交由调用者，VarDec只做插入符号表和返回Symbol的工作
/// @param isAssigned 变量是否被赋初值
/// @return Symbol*
Symbol *translate_vardec(ASTNode *vardec, Type type, int isAssigned) {
    if (match(vardec, 2, "VarDec", "ID")) {
        // VarDec -> ID
        // 每一个变量都需要与一个v%d绑定，v%d就是一个Operand
        char *var_name = vardec->child_list[0]->value.id;
        Symbol *sym = createVarSymbol(var_name, current_table->scope, type, vardec->lineno, isAssigned);
        add_symbol(current_table, sym);
        return sym;
    } else if (match(vardec, 5, "VarDec", "VarDec", "LB", "INT", "RB")) {
        // VarDec -> VarDec LB INT RB
        Type newType = createArrayType(type, vardec->child_list[2]->value.ival);
        return translate_vardec(vardec->child_list[0], newType, isAssigned);
    } else {
        Panic("Invalid VarDec");
    }
}

/// @brief 查全局表并插入"FUNCTION func"指令
void translate_fundec(ASTNode *fundec, Type retType) {
    char *func_name = fundec->child_list[0]->value.id;
    Operand func = new_func_op(func_name);
    insert_func_ir(func);  // 插入一条指令表示定义一个函数
    if (match(fundec, 5, "FunDec", "ID", "LP", "VarList", "RP")) {
        translate_varlist(fundec->child_list[2]);
    }
    Symbol *sym = lookup_symbol(global_table, func_name, 0, FUNC);
    Panic_ON(sym == NULL, "Function not found!");
    sym->operand = func;  // 只有在实验3才能修改Symbol的Operand
}

void translate_varlist(ASTNode *varlist) {
    if (match(varlist, 4, "VarList", "ParamDec", "COMMA", "VarList")) {
        translate_paramdec(varlist->child_list[0]);
        translate_varlist(varlist->child_list[2]);
    } else {
        translate_paramdec(varlist->child_list[0]);
    }
}

/// @brief 对于普通参数的Symbol，Operand绑定new_var_op()；对于数组或者结构体，绑定new_addr_op()。插入"PARAM x"
void translate_paramdec(ASTNode *paramdec) {
    Type type = translate_specifier(paramdec->child_list[0]);
    Symbol *sym = translate_vardec(paramdec->child_list[1], type, 0);
    if (sym->vartype->kind != BASIC) {
        sym->operand = new_addr_op();
    } else {
        sym->operand = new_var_op();
    }
    insert_param_ir(sym->operand);
}

/// @brief 跟实验2一样，Compst的部分子节点可能为空，因此注意判断
void translate_compst(ASTNode *compst, Type return_type) {
    for (int i = 0; i < compst->child_num; i++) {
        const char *chname = compst->child_list[i]->node_name;
        if (strcmp(chname, "DefList") == 0)
            translate_deflist(compst->child_list[i], NULL);
        else if (strcmp(chname, "StmtList") == 0)
            translate_stmtlist(compst->child_list[i], return_type);
    }
}

void translate_stmtlist(ASTNode *stmtlist, Type ret_type) {
    if (match(stmtlist, 3, "StmtList", "Stmt", "StmtList")) {
        translate_stmt(stmtlist->child_list[0], ret_type);
        translate_stmtlist(stmtlist->child_list[1], ret_type);
    } else {
        translate_stmt(stmtlist->child_list[0], ret_type);
    }
}

/// @brief 条件表达式翻译
/// @note 根据讲义P88，我们有五种情况:
/// 1. Exp -> Exp RELOP Exp ;
/// 2. Exp -> NOT Exp ;
/// 3. Exp -> Exp AND Exp ;
/// 4. Exp -> Exp OR Exp ;
/// 5. Exp -> Exp (必须是INT类型) ;
void translate_cond(ASTNode *exp, Operand label_true, Operand label_false) {
    if (match(exp, 4, "Exp", "Exp", "RELOP", "Exp")) {
        /**
         * Exp -> Exp RELOP Exp ::
         * If t1 op t2 GOTO label_true
         * GOTO label_false
         */
        Operand t1 = new_tmp_op();
        Operand t2 = new_tmp_op();
        translate_exp(exp->child_list[0], t1);
        translate_exp(exp->child_list[2], t2);
        insert_relop_ir(exp->child_list[1]->value.id, t1, t2, label_true);
        insert_goto_ir(label_false);
    } else if (match(exp, 3, "Exp", "NOT", "Exp")) {
        translate_cond(exp->child_list[1], label_false, label_true);
    } else if (match(exp, 4, "Exp", "Exp", "AND", "Exp")) {
        /**
         * Exp -> Exp AND Exp ::
         * 条件一成立，跳转到 lable_and,否则label_false
         * label_and:
         * 条件二成立，跳转到 label_true,否则label_false
         * label_false:
         * label_true:
         */
        Operand label_and = new_label();
        translate_cond(exp->child_list[0], label_and, label_false);
        insert_label_ir(label_and);
        translate_cond(exp->child_list[2], label_true, label_false);
    } else if (match(exp, 4, "Exp", "Exp", "OR", "Exp")) {
        /**
         * Exp -> Exp OR Exp ::
         * 条件一成立，跳转到 lable_true,否则label_or
         * label_or:
         * 条件二成立，跳转到 label_true,否则label_false
         * label_true:
         * label_false:
         */
        Operand label_or = new_label();
        translate_cond(exp->child_list[0], label_true, label_or);
        insert_label_ir(label_or);
        translate_cond(exp->child_list[2], label_true, label_false);
    } else {
        /**
         * Exp -> Exp (必须是INT类型) ::
         * If t1 != 0 GOTO label_true
         * GOTO label_false
         */
        Operand t1 = new_tmp_op();
        translate_exp(exp, t1);
        Operand zero = new_const(0);
        insert_relop_ir("!=", t1, zero, label_true);
        insert_goto_ir(label_false);
    }
}

/// @brief 翻译语句
void translate_stmt(ASTNode *stmt, Type ret_type) {
    if (match(stmt, 2, "Stmt", "CompSt")) {
        translate_compst(stmt->child_list[0], ret_type);
    } else if (match(stmt, 3, "Stmt", "Exp", "SEMI")) {
        // place赋空，所以小心段错误
        translate_exp(stmt->child_list[0], NULL);
    } else if (match(stmt, 4, "Stmt", "RETURN", "Exp", "SEMI")) {
        Operand ret = new_tmp_op();
        translate_exp(stmt->child_list[1], ret);
        insert_ret_ir(ret);
    } else if (match(stmt, 6, "Stmt", "IF", "LP", "Exp", "RP", "Stmt")) {
        Operand label1 = new_label();
        Operand label2 = new_label();
        translate_cond(stmt->child_list[2], label1, label2);
        insert_label_ir(label1);
        translate_stmt(stmt->child_list[4], ret_type);
        insert_label_ir(label2);
    } else if (match(stmt, 8, "Stmt", "IF", "LP", "Exp", "RP", "Stmt", "ELSE", "Stmt")) {
        /**
         * Stmt -> IF LP Exp RP Stmt1 ELSE Stmt2 ::
         * If Exp GOTO Label1 Else GOTO Label2
         * Label1:
         *      Stmt1
         *      GOTO Label3
         * Label2:
         *      Stmt2
         * Label3:
         * ...
         */
        Operand label1 = new_label();
        Operand label2 = new_label();
        Operand label3 = new_label();
        translate_cond(stmt->child_list[2], label1, label2);
        insert_label_ir(label1);
        translate_stmt(stmt->child_list[4], ret_type);
        insert_goto_ir(label3);
        insert_label_ir(label2);
        translate_stmt(stmt->child_list[6], ret_type);
        insert_label_ir(label3);
    } else if (match(stmt, 6, "Stmt", "WHILE", "LP", "Exp", "RP", "Stmt")) {
        /**
         * Stmt -> WHILE LP Exp RP Stmt ::
         * Label1:
         *      If Exp GOTO Label2 Else GOTO Label3
         * Label2:
         *      Stmt
         * GOTO Label1
         * Label3:
         * ...
         */
        Operand label1 = new_label();
        Operand label2 = new_label();
        Operand label3 = new_label();
        insert_label_ir(label1);
        translate_cond(stmt->child_list[2], label2, label3);
        insert_label_ir(label2);
        translate_stmt(stmt->child_list[4], ret_type);
        insert_goto_ir(label1);
        insert_label_ir(label3);
    } else {
        Panic("Invalid Stmt");
    }
}

/// @brief 参数fields在实验三无效（仅仅使用实验2的函数签名）
/// @param fields 无效参数，始终为NULL
void translate_deflist(ASTNode *deflist, FieldList *fields) {
    if (match(deflist, 2, "DefList", "Def")) {
        translate_def(deflist->child_list[0], fields);
    } else {
        translate_def(deflist->child_list[0], fields);
        translate_deflist(deflist->child_list[1], fields);
    }
}

/// @brief 参数fields在实验三无效（仅仅使用实验2的函数签名）
void translate_def(ASTNode *def, FieldList *fields) {
    ASTNode *specifier = def->child_list[0];
    Type type = translate_specifier(specifier);
    ASTNode *declist = def->child_list[1];
    translate_declist(declist, type, fields);
}

/// @brief 参数fields在实验三无效（仅仅使用实验2的函数签名）
void translate_declist(ASTNode *declist, Type type, FieldList *fields) {
    if (match(declist, 4, "DecList", "Dec", "COMMA", "DecList")) {
        translate_dec(declist->child_list[0], type);
        translate_declist(declist->child_list[2], type, fields);
    } else {
        translate_dec(declist->child_list[0], type);
    }
}

void translate_dec(ASTNode *dec, Type type) {
    if (match(dec, 2, "Dec", "VarDec")) {
        // Dec -> VarDec
        Symbol *sym = translate_vardec(dec->child_list[0], type, 0);
        // if (sym->vartype->kind == BASIC)
        sym->operand = new_var_op();  // 即使是数组和结构体,也只能对应到普通变量;作为函数参数时,二者才能是对应地址
        // else
        // sym->operand = new_addr_op();

        if (sym->vartype->kind != BASIC && struct_depth == 0) {
            insert_dec_ir(sym->operand, calculateMemSize(sym->vartype));
        }
    } else if (match(dec, 4, "Dec", "VarDec", "ASSIGNOP", "Exp")) {
        // Dec -> VarDec ASSIGNOP Exp
        Symbol *sym = translate_vardec(dec->child_list[0], type, 1);
        sym->operand = new_var_op();
        ASTNode *exp = dec->child_list[2];
        Operand place = new_tmp_op();
        translate_exp(exp, place);
        insert_assign_ir(ASS_NORMAL, sym->operand, place);
    } else {
        Panic("Invalid Dec");
    }
}

/// @brief 每一个表达式都会与一个Operand唯一绑定(作为参数place传递)
/// @return Type：在一些情况下需要，在一些情况下无效
Type translate_exp(ASTNode *exp, Operand place) {
    switch (exp->exp_type) {
        case ASSIGN_EXP: {
            // Exp -> Exp ASSIGNOP Exp
            exp_assign(exp, place);
            break;
        }
        case BINARY_EXP: {
            // Exp -> Exp AND Exp || Exp OR Exp || Exp RELOP Exp || Exp PLUS Exp || Exp MINUS Exp || Exp STAR Exp ||
            // Exp DIV Exp
            exp_binary(exp, place);
            break;
        }
        case UNARY_EXP: {
            // Exp -> MINUS Exp || NOT Exp
            Type ret = NULL;
            if (match(exp, 3, "Exp", "MINUS", "Exp")) {
                // Exp -> MINUS Exp
                Operand tmp = new_tmp_op();
                ret = translate_exp(exp->child_list[1], tmp);
                insert_binop_ir(IR_SUB, place, new_const(0), tmp);
            } else if (match(exp, 3, "Exp", "NOT", "Exp")) {
                // Exp -> NOT Exp
                Operand label1 = new_label();
                Operand label2 = new_label();
                insert_assign_ir(ASS_NORMAL, place, new_const(0));
                translate_cond(exp, label1, label2);
                insert_label_ir(label1);
                insert_assign_ir(ASS_NORMAL, place, new_const(1));
                insert_label_ir(label2);
            } else {
                Panic("Invalid Unary Exp");
            }
            return ret;
        }
        case FUN_EXP: {
            // Exp -> ID LP Args RP || Exp -> ID LP RP
            Symbol *fun = lookup_symbol(global_table, exp->child_list[0]->value.id, 0, FUNC);
            if (match(exp, 4, "Exp", "ID", "LP", "RP")) {
                // Exp -> ID LP RP
                if (strcmp(fun->name, "read") == 0) {
                    insert_read_ir(place);
                    break;
                }
                place = place == NULL ? new_tmp_op() : place;
                insert_call_ir(place, fun->operand);
            } else {
                // Exp -> ID LP Args RP
                if (strcmp(fun->name, "write") == 0) {
                    Operand tmp = new_tmp_op();
                    translate_exp(exp->child_list[2]->child_list[0], tmp);
                    insert_write_ir(tmp);
                    break;
                }
                translate_args(exp->child_list[2]);
                place = place == NULL ? new_tmp_op() : place;
                insert_call_ir(place, fun->operand);
            }
            return fun->returnType;
        }
        case ARR_EXP: {
            return exp_array(exp, place);
        }
        case STRU_EXP: {
            // Exp -> Exp DOT ID
            return exp_struct(exp, place);
        }
        case P_EXP: {
            // Exp -> LP Exp RP
            return translate_exp(exp->child_list[1], place);
        }
        case ID_EXP: {
            // Exp -> ID (通过查表将var)
            // ID的类型：
            // (1)普通变量 (2)结构体变量 (3) 数组变量
            // 对于普通变量，拷贝Operand
            // 对于结构体或者数组，需要考虑原先符号表中存储是OP_VAR还是OP_ADDR
            Symbol *sym = lookup_symbol(current_table, exp->child_list[0]->value.id, 1, VAR);

            switch (sym->vartype->kind) {
                case STRUCT: {
                    if (sym->operand->kind == OP_VARIABLE) {
                        insert_assign_ir(ASS_GETADDR, place, sym->operand);
                    } else {
                        dump_op(place, sym->operand);
                    }
                    break;
                }
                case ARRAY: {
                    if (sym->operand->kind == OP_VARIABLE) {
                        insert_assign_ir(ASS_GETADDR, place, sym->operand);
                    } else {
                        dump_op(place, sym->operand);
                    }
                    break;
                }
                case BASIC: {
                    dump_op(place, sym->operand);
                    break;
                }
                default: {
                    Panic("Invalid type");
                }
            }
            return sym->vartype;
        }
        case INT_EXP: {
            // Exp -> INT
            if (place) {
                place->kind = OP_CONSTANT;
                place->u.value = exp->child_list[0]->value.ival;
            }
            break;
        }
        case FLOAT_EXP: {
            // Exp -> FLOAT
            Panic("Invalid Float Exp");
        }
        default:
            Panic("Invalid Exp");
    }
    return createBasicType("int");  // 默认返回INT类型
}

/// @brief Exp -> Exp ASSIGN Exp (表达式左部有三种情况: ID, Exp[Exp],
/// Exp.ID)，所以本函数直接根据表达式左部展开式分为三类情况进行分析
/// @param exp ASTNode节点，place依旧是需要返回的Operand
void exp_assign(ASTNode *exp, Operand place) {
    Operand left = new_tmp_op();
    Operand right = new_tmp_op();

    if (match(exp->child_list[0], 2, "Exp", "ID")) {
        translate_exp(exp->child_list[0], left);
        translate_exp(exp->child_list[2], right);
        insert_assign_ir(ASS_NORMAL, left, right);
    } else if (match(exp->child_list[0], 4, "Exp", "Exp", "DOT", "ID")) {
        // 左边为结构变量的域时,我们希望解析左边表达式能够返回一个地址,这样我们才可以使用 *left = right
        left->u.tmp.tmp_addr = 1;
        translate_exp(exp->child_list[0], left);
        translate_exp(exp->child_list[2], right);
        insert_assign_ir(ASS_SETVAL, left, right);
    } else {
        // 左边为数组成员,我们希望解析左边的表达式能够返回一个地址,这样我们才可以使用 *left = right
        left->u.tmp.tmp_addr = 1;  // we are suppose to obtain the addr of the left exp
        translate_exp(exp->child_list[0], left);
        translate_exp(exp->child_list[2], right);
        insert_assign_ir(ASS_SETVAL, left, right);
    }

    // 因为不会出现数组和结构体的直接赋值，因此正确？
    if (place) {
        // 这里非常精妙，需要仔细思考和斟酌
        // 主要是考虑到多个赋值连续的情况，注意赋值号解析是从右往左的
        int left_is_addr = IS_TMP_ADDR(left) || (left->kind == OP_ADDRESS);
        if (!left_is_addr)  // 如果是普通变量的赋值，left存放值，place := left
            insert_assign_ir(ASS_NORMAL, place, left);
        else  // 如果是数组或者结构体赋值，那么left存放地址，需要place := *left
            insert_assign_ir(ASS_GETVAL, place, left);
    }
}

/// @brief 结构体绑定的操作数(place)Operand类型必定为临时变量并且tmp_addr = 1表示临时变量存储地址
/// @param place exp绑定的操作数，本次解析需要返回的内容
/// @return May not be used
/// @note 如果place->tmp_addr = 0表示调用者希望我们返回一个值
Type exp_struct(ASTNode *exp, Operand place) {
    // Exp -> Exp.ID
    char *field_name = exp->child_list[2]->value.id;
    Operand basis = new_tmp_op();
    basis->u.tmp.tmp_addr = 1;  // 希望拿到地址
    Type type = translate_exp(exp->child_list[0], basis);
    int offset = get_offset_in_struct(type, field_name);
    Type ret = findField(type, field_name);

    int should_ret_val = (IS_TMP_VAL(place)) && (ret->kind == BASIC);  // place需要返回值
    if (should_ret_val) {
        Operand addr = new_tmp_op();
        insert_binop_ir(IR_ADD, addr, basis, new_const(offset));  // addr := basis + #offset
        insert_assign_ir(ASS_GETVAL, place, addr);                // place = *addr
    } else {
        insert_binop_ir(IR_ADD, place, basis, new_const(offset));
    }
    return ret;
}

/// @brief 数组绑定的操作数(place)Operand类型必定为临时变量并且tmp_addr = 1表示临时变量存储地址
/// @param place exp绑定的操作数
/// @return May not be used
/// @note 如果place->tmp_addr = 0表示调用者希望我们返回一个值
Type exp_array(ASTNode *exp, Operand place) {
    if (place == NULL) place = new_tmp_op();  // 默认返回值比如a[func()];这种语句我们希望func()函数还是会被调用
    Panic_ON(match(exp, 5, "Exp", "Exp", "LB", "Exp", "RB") == 0, "exp_array");
    /**
     * 为了统一，我们约定place为临时变量并且存储地址 kind = OP_TEMP && u.tmp_addr = 1
     * 整个过程比较简单：
     * （1）获得数组基地址basis : translate_exp(child[0], basis)
     * （2）获得索引index : translate_exp(child[2], index)
     * （3）获得偏移量offset:
     *      (3.1) index->kind = OP_CONST, offset = new_const(index->value * elem.memSize)
     *      (3.2) 否则，插入乘法指令：offset := index * elem.memSize
     * 去除place存储地址的假设，我们需要考虑读取数组元素（比如出现在=右边并且参与计算时），我们希望place能够返回一个值而非地址，因此：
     *      当place->tmp_addr = 0（由调用者告知）时，我们需要再插入 y := *addr指令
     */
    Operand basis = new_tmp_op();
    basis->u.tmp.tmp_addr = 1;
    Type type = translate_exp(exp->child_list[0], basis);  // first we obtain the basis addr of the array
    Operand index = new_tmp_op();
    translate_exp(exp->child_list[2], index);
    Operand offset;
    if (index->kind == OP_CONSTANT) {
        offset = new_const(index->u.value *
                           calculateMemSize(type->u.array.elem));  // 记得调用函数计算元素大小（否则高维数组会出错）
    } else {
        offset = new_tmp_op();
        insert_binop_ir(IR_MUL, offset, index, new_const(calculateMemSize(type->u.array.elem)));
    }

    int should_ret_val =
        (IS_TMP_VAL(place) &&
         type->u.array.elem->kind == BASIC);  // 判断接受值:只有tmp_flag = 0并且是解析完数组到最底层才返回数组的值
    if (should_ret_val) {
        Operand addr = new_tmp_op();
        insert_binop_ir(IR_ADD, addr, basis, offset);  // addr := basis + offset
        insert_assign_ir(ASS_GETVAL, place, addr);     // place := *addr
    } else {
        insert_binop_ir(IR_ADD, place, basis, offset);  // place := basis + offset
    }
    return type->u.array.elem;
}

/// @brief Exp -> Exp AND Exp || Exp OR Exp || Exp RELOP Exp || Exp PLUS Exp || Exp MINUS Exp || Exp STAR Exp || Exp
/// DIV Exp
/// @param exp ASTNode节点
void exp_binary(ASTNode *exp, Operand place) {
    // if (place == NULL) return;  // ex : 1 + 1;之类的语句，无实际含义可以直接返回
    Operand left = new_tmp_op();
    Operand right = new_tmp_op();
    int kind = -1;
    switch (exp->bi_type) {
        case BI_AND:
        case BI_OR:
        case BI_RELOP: {
            Operand label1 = new_label();
            Operand label2 = new_label();

            insert_assign_ir(ASS_NORMAL, place, new_const(0));
            translate_cond(exp, label1, label2);
            insert_label_ir(label1);
            insert_assign_ir(ASS_NORMAL, place, new_const(1));
            insert_label_ir(label2);

            return;
        }
        case BI_PLUS:
            kind = IR_ADD;  // 中间代码的类型
            break;
        case BI_MINUS:
            kind = IR_SUB;
            break;
        case BI_STAR:
            kind = IR_MUL;
            break;
        case BI_DIV:
            kind = IR_DIV;
            break;
        default:
            break;
    }
    translate_exp(exp->child_list[0], left);
    translate_exp(exp->child_list[2], right);
    insert_binop_ir(kind, place, left, right);  // ex : t1 := v2 + #1
}

/// @brief 注意到参数的push顺序，比如func(x, y, z)那么指令为: ARG z; ARG y; ARG x
void translate_args(ASTNode *args) {
    if (match(args, 2, "Args", "Exp")) {
        // Args -> Exp
        Operand arg = new_tmp_op();
        translate_exp(args->child_list[0], arg);
        insert_arg_ir(arg);
    } else if (match(args, 4, "Args", "Exp", "COMMA", "Args")) {
        // Args -> Exp COMMA Args
        Operand arg = new_tmp_op();
        translate_exp(args->child_list[0], arg);
        translate_args(args->child_list[2]);
        insert_arg_ir(arg);
    } else {
        Panic("Invalid Args");
    }
}

//============================================================================================================================//
//------------------------- 打印指令和文件输出相关 ----------------------------------//

/// @brief 将Operand转换为字符串
/// @note
/// Operand的可能值为变量、临时变量、常量、地址，格式分别为v1、t1(注意到临时变量可以存储值或者地址)、#1、v1，至于什么时候用&t||*t或者&v||*v则取决于具体的指令，本函数不需要考虑这个问题
/// @param op Operand
/// @return 字符串
char *print_operand(Operand op) {
    if (op == NULL) return NULL;  // 这是因为有一些函数调用时可以不使用返回值，比如write(x)
    char *str = (char *)malloc(16);
    switch (op->kind) {
        case OP_VARIABLE:
            sprintf(str, "v%d", op->u.var_no);
            break;
        case OP_TEMP:
            sprintf(str, "t%d", op->u.tmp.tmp_no);
            break;
        case OP_CONSTANT:
            sprintf(str, "#%d", op->u.value);
            break;
        case OP_ADDRESS:
            sprintf(str, "v%d", op->u.var_no);
            break;
        case OP_FUNCTION:
            sprintf(str, "%s", op->u.func_name);
            break;
        default:
            sprintf(str, "unknown");
            break;
    }
    return str;
}

/// @brief 将中间代码输出到文件中
/// @param fp 文件指针
void print_intercodes(FILE *fp) {
    InterCodes p = head;
    while (p != NULL) {
        switch (p->code->kind) {
            case IR_LABEL:
                fprintf(fp, "LABEL label%d :\n", p->code->u.one.op->u.var_no);
                break;
            case IR_FUNCTION:
                fprintf(fp, "FUNCTION %s :\n", p->code->u.one.op->u.func_name);
                break;
            case IR_ASSIGN: {
                char *lop = print_operand(p->code->u.assign.left);
                char *rop = print_operand(p->code->u.assign.right);
                int ass_type = p->code->u.assign.ass_type;
                switch (ass_type) {
                    case ASS_NORMAL: {
                        fprintf(fp, "%s := %s\n", lop, rop);
                        break;
                    }
                    case ASS_GETADDR: {
                        fprintf(fp, "%s := &%s\n", lop, rop);
                        break;
                    }
                    case ASS_SETVAL: {
                        fprintf(fp, "*%s := %s\n", lop, rop);
                        break;
                    }
                    case ASS_GETVAL: {
                        fprintf(fp, "%s := *%s\n", lop, rop);
                        break;
                    }
                    default:
                        Panic("Not imple");
                }
                break;
            }
            case IR_ADD: {
                char *x = print_operand(p->code->u.binop.result);
                char *y = print_operand(p->code->u.binop.op1);
                char *z = print_operand(p->code->u.binop.op2);
                fprintf(fp, "%s := %s + %s\n", x, y, z);
                break;
            }
            case IR_ADDRADD: {
                // x = &y + z
                char *x = print_operand(p->code->u.binop.result);
                char *y = print_operand(p->code->u.binop.op1);
                char *z = print_operand(p->code->u.binop.op2);
                fprintf(fp, "%s := &%s + %s\n", x, y, z);
                break;
            }
            case IR_SUB: {
                char *x = print_operand(p->code->u.binop.result);
                char *y = print_operand(p->code->u.binop.op1);
                char *z = print_operand(p->code->u.binop.op2);
                fprintf(fp, "%s := %s - %s\n", x, y, z);
                break;
            }
            case IR_MUL: {
                char *x = print_operand(p->code->u.binop.result);
                char *y = print_operand(p->code->u.binop.op1);
                char *z = print_operand(p->code->u.binop.op2);
                fprintf(fp, "%s := %s * %s\n", x, y, z);
                break;
            }
            case IR_DIV: {
                char *x = print_operand(p->code->u.binop.result);
                char *y = print_operand(p->code->u.binop.op1);
                char *z = print_operand(p->code->u.binop.op2);
                fprintf(fp, "%s := %s / %s\n", x, y, z);
                break;
            }
            case IR_GOTO: {
                fprintf(fp, "GOTO label%d\n", p->code->u.one.op->u.var_no);
                break;
            }
            case IR_IFGOTO: {
                // IF x relop y GOTO z
                char *x = print_operand(p->code->u.ifgoto.x);
                char *y = print_operand(p->code->u.ifgoto.y);
                fprintf(fp, "IF %s %s %s GOTO label%d\n", x, p->code->u.ifgoto.relop, y, p->code->u.ifgoto.z->u.var_no);
                break;
            }
            case IR_RETURN: {
                // RETURN x
                char *one = print_operand(p->code->u.one.op);
                fprintf(fp, "RETURN %s\n", one);
                break;
            }
            case IR_DEC: {
                // DEC x size
                char *x = print_operand(p->code->u.dec.op);
                fprintf(fp, "DEC %s %d\n", x, p->code->u.dec.size);
                break;
            }
            case IR_ARG: {
                // ARG x
                char *arg = print_operand(p->code->u.one.op);
                fprintf(fp, "ARG %s\n", arg);
                break;
            }
            case IR_CALL: {
                // x := CALL f
                char *x = print_operand(p->code->u.two.left);
                char *f = print_operand(p->code->u.two.right);
                fprintf(fp, "%s := CALL %s\n", x, f);
                break;
            }
            case IR_PARAM: {
                // PARAM x
                Operand op = p->code->u.one.op;
                char *str = (char *)malloc(16);
                sprintf(str, "v%d", op->u.var_no);
                fprintf(fp, "PARAM %s\n", str);
                free(str);
                break;
            }
            case IR_READ: {
                char *x = print_operand(p->code->u.one.op);
                fprintf(fp, "READ %s\n", x);
                break;
            }
            case IR_WRITE: {
                char *x = print_operand(p->code->u.one.op);
                fprintf(fp, "WRITE %s\n", x);
                break;
            }
            default:
                Panic("Invalid InterCode");
                break;
        }
        p = p->next;
    }
}