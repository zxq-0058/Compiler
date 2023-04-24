/*
* syntax.y :: Parse definition for the c-- language
*/
%locations
%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdarg.h>
    #include "syntax.h"
    #include "logger.h"
    #include "lex.yy.c"
    
    #ifdef YYDEBUG
    #undef YYDEBUG
    #endif

//     #define YYDEBUG 1

    extern int yylex();           /*  the entry point to the lexer  */
    int syntax_error = 0;         /* indicating whether syntax errors occur */
    void yyerror(const char* s) {
        syntax_error = 1;
        fprintf(stdout, "Error type B at Line %d: %s.\n", yylineno, s);
    }       /* errors reporting */
    extern int lexical_error;     /* indicating whether lexical errors occur(lexical.l)*/
%}

%union{
    ASTNode* ast_node;
}

/* Keywords: use newKeyword(int key_id) */
%token <ast_node> STRUCT RETURN IF ELSE WHILE
/* newSEMI(), newCOMMA() */
%token <ast_node> SEMI COMMA
/* newTYPE(char * type) where type in ["int", "float"] */
%token <ast_node> TYPE
/* Opterators: use newOperator(int op_type) */
%token <ast_node> ASSIGNOP RELOP PLUS MINUS STAR DIV AND OR DOT NOT LP RP LB RB LC RC
/* Identifiers: use newID(char *id_name) */
%token <ast_node>ID
/* Int or float const: use newInt() and newFloat() */
%token <ast_node> INT
%token <ast_node> FLOAT 

/* Operator priority */
%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT UMINUS
%left LP RP LB RB DOT
/* %left ELSE */
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

/* Non-terminals */
%type<ast_node> Epsilon /* to match empty rules */
%type<ast_node> Program ExtDefList ExtDef ExtDecList Function_Declaration Function_Definition
%type<ast_node> Specifier StructSpecifier OptTag Tag
%type<ast_node> VarDec FunDec VarList ParamDec
%type<ast_node> CompSt StmtList Stmt
%type<ast_node> DefList Def DecList Dec
%type<ast_node> Exp Args


%define parse.error verbose
/* %destructor { Panic("TODO:free the Node!"); } <ast_node> */

%start Program

%%
/* Rules section */

/* High-level definitions */
Epsilon: {$$ = NULL;}/* Empty rule */
;

Program: ExtDefList {  
        $$ = newProgram(@1.first_line, $1);
        };

ExtDefList: ExtDef ExtDefList {
        $$ = newASTNode("ExtDefList", @1.first_line, 2, $1, $2);
        }
        | Epsilon;

/* 函数声明 */
Function_Declaration: Specifier FunDec SEMI { $$ = newASTNode("Function_Declaration", @1.first_line, 3, $1, $2, $3); }
/* 函数定义 */
Function_Definition: Specifier FunDec CompSt { $$ = newASTNode("Function_Definition", @1.first_line, 3, $1, $2, $3); }


ExtDef: Specifier ExtDecList SEMI {
        /* Global var */
        Log("Global var matched!");
        $$ = newASTNode("ExtDef", @1.first_line, 3, $1, $2, $3);
        }
        | Specifier SEMI{
        /* Struct def */
        Log("Struct def matched!");
        $$ = newASTNode("ExtDef", @1.first_line, 2, $1, $2);
        }
        | Function_Declaration {
        $$ = newASTNode("ExtDef", @1.first_line, 1, $1);
        }
        | Function_Definition {
        $$ = newASTNode("ExtDef", @1.first_line, 1, $1);
        }
        | Specifier ExtDecList error {
        $$ = NULL;
        Warn("Global Definition Likely Missing ';'");
        yyerrok;      
        };
        | Specifier error {
        $$ = NULL;
        Warn("Struct Definition Likely Missing ';'");
        yyerrok;      
        };
        | error SEMI {
        $$ = NULL;
        // yyerrok说明不需要丢弃任何符号，在分号之后重新进行分析
        yyerrok;
        };

ExtDecList: VarDec {
        $$ = newASTNode("ExtDecList", @1.first_line, 1, $1);
        }
        | VarDec COMMA ExtDecList {
        $$ = newASTNode("ExtDecList", @1.first_line, 3, $1, $2, $3);
        }
        | VarDec COMMA error {
        $$ = NULL;
        Warn("");
        }
        | VarDec error ExtDecList {
        $$ = NULL;
        Warn("Should be ',' between vars");
        }
        ;

/* Specifiers */
Specifier: TYPE {
        $$ = newASTNode("Specifier", @1.first_line, 1, $1);
        } 
        | StructSpecifier {
        $$ = newASTNode("Specifier", @1.first_line, 1, $1);
        }
        ;

StructSpecifier : STRUCT OptTag LC DefList RC {
        $$ = newASTNode("StructSpecifier", @1.first_line, 5, $1, $2, $3, $4, $5);
        }
        | STRUCT Tag {
        $$ = newASTNode("StructSpecifier", @1.first_line, 2, $1, $2);
        }
        | STRUCT OptTag LC error {yyerrok;};
        | STRUCT OptTag LC error RC {yyerrok;};
        /* | error RC { yyerrok; } */
        ;

OptTag : ID {
        $$ = newASTNode("OptTag", @1.first_line, 1, $1);
        }
        | Epsilon;

Tag : ID {
        $$ = newASTNode("Tag", @1.first_line, 1, $1);
        };

/* Declarators */
VarDec: ID { $$ = newASTNode("VarDec", @1.first_line, 1, $1); }
        | VarDec LB INT RB { $$ = newASTNode("VarDec", @1.first_line, 4, $1, $2, $3, $4); } 
        | VarDec LB error RB { syntax_error = 1; yyerrok;};

FunDec: ID LP VarList RP { $$ = newASTNode("FunDec", @1.first_line, 4, $1, $2, $3, $4); }
        | ID LP RP { $$ = newASTNode("FunDec", @1.first_line, 3, $1, $2, $3); };
        | error RP { yyerrok; } ;

VarList: ParamDec COMMA VarList { $$ = newASTNode("VarList", @1.first_line, 3, $1, $2, $3); }
        | ParamDec { $$ = newASTNode("VarList", @1.first_line, 1, $1); } ;
        | ParamDec COMMA error { $$ = NULL; yyerrok;} ; // 缺失逗号 int main(int i, int j, )
        | ParamDec error VarList { $$ = NULL; yyerrok; }; // 缺失逗号 int main(int i int j)

ParamDec: Specifier VarDec{ $$ = newASTNode("ParamDec", @1.first_line, 2, $1, $2); } ;
        | Specifier error {}; // int j i
        | error VarDec {} // int int i
        ;

/* Statements */
CompSt: LC DefList StmtList RC { $$ = newASTNode("CompSt", @1.first_line, 4, $1, $2, $3, $4); }
        | error RC { yyerrok; } ; // 右括号缺失正常处理

StmtList: Stmt StmtList { $$ = newASTNode("StmtList", @1.first_line, 2, $1, $2); }
        | Epsilon ;

Stmt: Exp SEMI { $$ = newASTNode("Stmt", @1.first_line, 2, $1, $2); }
        | CompSt { $$ = newASTNode("Stmt", @1.first_line, 1, $1); }
        | RETURN Exp SEMI { $$ = newASTNode("Stmt", @1.first_line, 3, $1, $2, $3); }
        | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE{ $$ = newASTNode("Stmt", @1.first_line, 5, $1, $2, $3, $4, $5); }
        | IF LP Exp RP Stmt ELSE Stmt { $$ = newASTNode("Stmt", @1.first_line, 7, $1, $2, $3, $4, $5, $6, $7); }
        | WHILE LP Exp RP Stmt { $$ = newASTNode("Stmt", @1.first_line, 5, $1, $2, $3, $4, $5); }
        | Exp error { yyerrok; }
        | RETURN Exp error { yyerrok; } 
        | error SEMI { yyerrok; } ;

/* Local Definitions */
DefList: Def DefList {
        $$ = newASTNode("DefList", @1.first_line, 2, $1, $2);
        } | Epsilon ;

Def: Specifier DecList SEMI{ $$ = newASTNode("Def", @1.first_line, 3, $1, $2, $3); }
        | Specifier DecList error { Log("局部变量声明出错\n"); yyerrok;}
        | Specifier error {yyerrok;}
        /* | error SEMI { Log("局部变量声明出错，假装归纳到Def(局部变量的声明)"); yyerrok; } ; */

DecList: Dec { $$ = newASTNode("DecList", @1.first_line, 1, $1); } 
        | Dec COMMA DecList { $$ = newASTNode("DecList", @1.first_line, 3, $1, $2, $3); }; 
        | Dec COMMA error {} // 多余逗号

Dec: VarDec { $$ = newASTNode("Dec", @1.first_line, 1, $1); } 
        | VarDec ASSIGNOP Exp { $$ = newASTNode("Dec", @1.first_line, 3, $1, $2, $3); };
        | error ASSIGNOP Exp {
        $$ = NULL;
        Warn("Error in declaration: missing variable");
        yyerrok;
        };

/* Expressions */
Exp: Exp ASSIGNOP Exp { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); $$->exp_type = ASSIGN_EXP;}
        | Exp AND Exp { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); $$->exp_type = BINARY_EXP; $$->bi_type = BI_AND; }
        | Exp OR Exp  { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); $$->exp_type = BINARY_EXP; $$->bi_type = BI_OR; }
        | Exp RELOP Exp { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3);$$->exp_type = BINARY_EXP; $$->bi_type = BI_RELOP;}
        | Exp PLUS Exp { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); $$->exp_type = BINARY_EXP; $$->bi_type = BI_PLUS;}
        | Exp MINUS Exp { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); $$->exp_type = BINARY_EXP; $$->bi_type = BI_MINUS;}
        | Exp STAR Exp { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); $$->exp_type = BINARY_EXP; $$->bi_type = BI_STAR;}
        | Exp DIV Exp { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); $$->exp_type = BINARY_EXP; $$->bi_type = BI_DIV;}
        | LP Exp RP { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); $$->exp_type = P_EXP;}
        | MINUS Exp %prec UMINUS { $$ = newASTNode("Exp", @1.first_line, 2, $1, $2); $$->exp_type = UNARY_EXP;}
        | NOT Exp{ $$ = newASTNode("Exp", @1.first_line, 2, $1, $2); $$->exp_type = UNARY_EXP;}
        | ID LP Args RP { $$ = newASTNode("Exp", @1.first_line, 4, $1, $2, $3, $4); $$->exp_type = FUN_EXP;}
        | ID LP RP { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3);$$->exp_type = FUN_EXP;}
        | Exp LB Exp RB { $$ = newASTNode("Exp", @1.first_line, 4, $1, $2, $3, $4); $$->exp_type = ARR_EXP;}
        | Exp DOT ID { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3);$$->exp_type = STRU_EXP;}
        | ID {  $$ = newASTNode("Exp", @1.first_line, 1, $1);$$->exp_type = ID_EXP;}
        | INT { $$ = newASTNode("Exp", @1.first_line, 1, $1);$$->exp_type = INT_EXP;}
        | FLOAT { $$ = newASTNode("Exp", @1.first_line, 1, $1); $$->exp_type = FLOAT_EXP;}
        | error RP { yyerrok; } ;

Args: Exp COMMA Args { $$ = newASTNode("Args", @1.first_line, 3, $1, $2, $3); }
        | Exp { $$ = newASTNode("Args", @1.first_line, 1, $1); } 
        | error COMMA Args {
                $$ = NULL;
                Warn("Error in function arguments: missing argument");
                yyerrok;
        };

%%


ASTNode *ast_root = NULL; // to be initialized in newProgram()


ASTNode *newASTNode(const char *name, int lineno, int numChildren, ...)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->node_name = name;
    node->lineno = lineno;
    node->value_type = NONE;
    Log("node %s at line::%d", name, lineno);
    node->child_num = 0;
    va_list children;
    va_start(children, numChildren);
    for (int i = 0; i < numChildren; i++)
    {
        ASTNode *child = va_arg(children, struct ASTNode *);
        if (child == NULL) continue;
        add_child(node, child);
    }
    va_end(children);
    return node;
}

void add_child(ASTNode *parent, ASTNode *child)
{
    // 这里其实处理的不太好，因为子节点可能是NULL
    Panic_on(parent == NULL || child == NULL, "NULL node!");
    parent->child_list[parent->child_num] = child;
    parent->child_num++;
}

ASTNode *newProgram(int lineno, ASTNode *ExtDefList)
{
     if (ast_root) {
        Panic("ast_root has been initialized!");
        return NULL;
     }
    if (ExtDefList == NULL) {
        ast_root = NULL;
    } else {
        ast_root = newASTNode("Program", lineno, 1, ExtDefList);
    }
    return ast_root;
}

ASTNode *newTYPE(const char* type) {
     ASTNode *n = newASTNode("TYPE", -1, 0);
     n->value_type = TYPE_;
     strcpy(n->value.type, type);
     return n;
} 


ASTNode *newID(char *id_name) {
     Log("id_name %s", id_name);
     ASTNode *id = newASTNode("ID", -1, 0);
     id->value_type = ID_;
     strcpy(id->value.id, id_name);
     return id;
}

typedef struct table_t {
        int type;
        char name[16];
}table_t;

static table_t op_table[] = {
        {ASSIGNOP, "ASSIGNOP"},
        {RELOP, "RELOP"},
        {PLUS , "PLUS"},
        {MINUS, "MINUS"},
        {STAR, "STAR"},
        {DIV, "DIV"},
        {AND, "AND"},
        {OR, "OR"},
        {DOT, "DOT"},
        {NOT, "NOT"},
        {LP, "LP"},
        {RP, "RP"},
        {LB, "LB"},
        {RB, "RB"},
        {LC, "LC"},
        {RC, "RC"},
};
ASTNode *newOperator(int op_type) {
     Log("The optype is %d", op_type);
     int num = sizeof(op_table) / sizeof(table_t);
     int i;
     for(i = 0; i < num; i++) {
        if (op_type == op_table[i].type) {
               break;
        }
     }
     Panic_on(i == num, "Table search failed!");
     ASTNode *op = newASTNode(op_table[i].name, -1, 0);
     return op;
}

ASTNode *newInt(unsigned int _int) {
     ASTNode *i = newASTNode("INT", -1, 0);
     i->value_type = INT_;
     i->value.ival = _int;
     return i;
}

ASTNode *newFloat(float _float) {
     ASTNode *f = newASTNode("FLOAT", -1, 0);
     f->value_type = FLOAT_;
     f->value.fval = _float;
     return f;
}

ASTNode *newSEMI() {
    return newASTNode("SEMI", -1, 0);
}

ASTNode *newCOMMA() {
    return newASTNode("COMMA", -1, 0);
}

/* key words in C-- */
static table_t keywords[] = {
        {STRUCT, "STRUCT"},
        {RETURN, "RETURN"},
        {IF, "IF"},
        {ELSE, "ELSE"},
        {WHILE, "WHILE"}
};

ASTNode *newKeyword(int key) {
     int num = sizeof(keywords) / sizeof(table_t);
     int i;
     for(i = 0; i < num; i++) {
        if (key == keywords[i].type) {
               break;
        }
     }
     Panic_on(i == num, "Table search failed!");
     ASTNode *key_node = newASTNode(keywords[i].name, -1, 0);
     return key_node;
}

void print_AST()
{
    if(syntax_error || lexical_error) {
        return ;
    }
    dfs_print(ast_root, 0);
}

void dfs_print(ASTNode *node, int depth) {
    for(int i = 0; i < depth; i++) {
        printf("  "); // indentation
    }
    Panic_on(node == NULL, "Null node!");
    printf("%s", node->node_name);
    if (node->lineno != -1) {
        printf(" (%d)", node->lineno);
    }
    if (node->value_type != NONE) {
        printf(": ");
        switch(node->value_type) {
                case ID_ : printf("%s", node->value.id); break;
                case TYPE_: printf("%s", node->value.type); break;
                case INT_ :printf("%u", node->value.ival); break;
                case FLOAT_: printf("%f", node->value.fval); break;
                default: Panic("Invalid value type!");
        }
    }
    printf("\n");
    for(int i = 0; i < node->child_num; i++) {
        dfs_print(node->child_list[i], depth + 1);
    }
}
