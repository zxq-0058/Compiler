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
    
    extern int yylex();           /*  the entry point to the lexer  */
    void yyerror(char* s);        /* errors reporting */
    int syntax_error = 0;         /* indicating whether syntax errors occur */
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
%right NOT
%left LP RP LB RB DOT
/* %left ELSE */
%nonassoc LOWER_THAN_ELSE


/* Non-terminals */
%type<ast_node> Epsilon /* to match empty rules */
%type<ast_node> Program ExtDefList ExtDef ExtDecList
%type<ast_node> Specifier StructSpecifier OptTag Tag
%type<ast_node> VarDec FunDec VarList ParamDec
%type<ast_node> CompSt StmtList Stmt
%type<ast_node> DefList Def DecList Dec
%type<ast_node> Exp Args

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
        | Specifier FunDec CompSt{
        $$ = newASTNode("ExtDef", @1.first_line, 3, $1, $2, $3); 
        }
        | error SEMI {
        yyerrok;
        };

ExtDecList: VarDec {
        $$ = newASTNode("ExtDecList", @1.first_line, 1, $1);
        }
        | VarDec COMMA ExtDecList {
        $$ = newASTNode("VarDec", @1.first_line, 3, $1, $2, $3);
        };

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
        | error RC { yyerrok; }
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
        | VarDec LB error RB { syntax_error = 1; yyerrok; yyerror("Missing ']'\n"); };

FunDec: ID LP VarList RP { $$ = newASTNode("FunDec", @1.first_line, 4, $1, $2, $3, $4); }
        | ID LP RP { $$ = newASTNode("FunDec", @1.first_line, 3, $1, $2, $3); }
        | error RP { yyerrok; } ;

VarList: ParamDec COMMA VarList { $$ = newASTNode("VarList", @1.first_line, 3, $1, $2, $3); }
        | ParamDec { $$ = newASTNode("VarList", @1.first_line, 1, $1); } ;

ParamDec: Specifier VarDec{ $$ = newASTNode("ParamDec", @1.first_line, 2, $1, $2); } 
        /* | error COMMA { syntax_error = 1; yyerrok("Parameter Dec Missing ';'\n");} */
        ;

/* Statements */
CompSt: LC DefList StmtList RC { $$ = newASTNode("CompSt", @1.first_line, 4, $1, $2, $3, $4); }
        | error RC { yyerrok; } ;

StmtList: Stmt StmtList { $$ = newASTNode("StmtList", @1.first_line, 2, $1, $2); }
        | Epsilon ;

Stmt: Exp SEMI { $$ = newASTNode("Stmt", @1.first_line, 2, $1, $2); }
        | CompSt { $$ = newASTNode("Stmt", @1.first_line, 1, $1); }
        | RETURN Exp SEMI { $$ = newASTNode("Stmt", @1.first_line, 3, $1, $2, $3); }
        | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE{ $$ = newASTNode("Stmt", @1.first_line, 5, $1, $2, $3, $4, $5); }
        | IF LP Exp RP Stmt ELSE Stmt { $$ = newASTNode("Stmt", @1.first_line, 7, $1, $2, $3, $4, $5, $6, $7); }
        | WHILE LP Exp RP Stmt { $$ = newASTNode("Stmt", @1.first_line, 5, $1, $2, $3, $4, $5); }
        | error SEMI { yyerror("Missing ';'\n"); yyerrok; } ;

/* Local Definitions */
DefList: Def DefList {
        $$ = newASTNode("DefList", @1.first_line, 2, $1, $2);
        } | Epsilon ;

Def: Specifier DecList SEMI{ $$ = newASTNode("Def", @1.first_line, 3, $1, $2, $3); }
        | error SEMI { yyerrok; } ;

DecList: Dec { $$ = newASTNode("DecList", @1.first_line, 1, $1); } 
        | Dec COMMA DecList { $$ = newASTNode("DecList", @1.first_line, 3, $1, $2, $3); }; 

Dec: VarDec { $$ = newASTNode("Dec", @1.first_line, 1, $1); } 
        | VarDec ASSIGNOP Exp { $$ = newASTNode("Dec", @1.first_line, 3, $1, $2, $3); };

/* Expressions */
Exp: Exp ASSIGNOP Exp { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); }
        | Exp AND Exp { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); }
        | Exp OR Exp  { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); }
        | Exp RELOP Exp { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); }
        | Exp PLUS Exp { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); }
        | Exp MINUS Exp { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); }
        | Exp STAR Exp { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); }
        | Exp DIV Exp { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); }
        | LP Exp RP { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); }
        | MINUS Exp { $$ = newASTNode("Exp", @1.first_line, 2, $1, $2); }
        | NOT Exp{ $$ = newASTNode("Exp", @1.first_line, 2, $1, $2); }
        | ID LP Args RP { $$ = newASTNode("Exp", @1.first_line, 4, $1, $2, $3, $4); }
        | ID LP RP { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); }
        | Exp LB Exp RB { $$ = newASTNode("Exp", @1.first_line, 4, $1, $2, $3, $4); }
        | Exp DOT ID { $$ = newASTNode("Exp", @1.first_line, 3, $1, $2, $3); }
        | ID {  $$ = newASTNode("Exp", @1.first_line, 1, $1); }
        | INT { $$ = newASTNode("Exp", @1.first_line, 1, $1); }
        | FLOAT { $$ = newASTNode("Exp", @1.first_line, 1, $1); }
        | error RP { yyerrok; } ;

Args: Exp COMMA Args { $$ = newASTNode("Args", @1.first_line, 3, $1, $2, $3); }
        | Exp { $$ = newASTNode("Args", @1.first_line, 1, $1); } ;

%%

void yyerror(char* s) {
    syntax_error = 1;
    fprintf(stdout, "Error type B at Line %d: %s.\n", yylineno, s);
}

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
     ast_root = newASTNode("Program", lineno, 1, ExtDefList);
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

ASTNode *newInt(int _int) {
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
                case INT_ :printf("%d", node->value.ival); break;
                case FLOAT_: printf("%f", node->value.fval); break;
                default: Panic("Invalid value type!");
        }
    }
    printf("\n");
    for(int i = 0; i < node->child_num; i++) {
        dfs_print(node->child_list[i], depth + 1);
    }
}