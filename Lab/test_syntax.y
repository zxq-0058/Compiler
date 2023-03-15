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
    void yyerror(char* s, ...);
%}

// TODO: Be careful, may not be complete
%union{
    int int_val;
    float float_val;
    char* string_val;
    char* error_msg;
    ASTNode* ast_node; // 使用指针
//     struct symbol *_symbol; // TODO:: 处理id时可以考虑使用strdup函数
}
%token STRUCT RETURN IF ELSE WHILE
%token SEMI COMMA
%token TYPE
%token LC RC
%token <string_val>ID
// CONST:: TODO add string CONST
%token <int_val> INT
%token <float_val> FLOAT 


// TODO: neg is not taken into consideration
%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT
%left LP RP LB RB DOT

%token ERROR

%type<ast_node> Program ExtDefList ExtDef Epsilon ExtDecList
%type<ast_node> Specifier StructSpecifier OptTag Tag
%type<ast_node> VarDec FunDec VarList ParamDec
%type<ast_node> CompSt StmtList Stmt
%type<ast_node> DefList Def DecList Dec
%type<ast_node> Exp Args

/* %start Specifier */
%start Def
%%
/* rules section */

/* High-level definitions */
Epsilon: {$$ = NULL;}/* Empty rule */
;

/* Specifiers */
Specifier:StructSpecifier {
        $$ = newASTNode("Specifier", @1.first_line, 1, $1);
        ast_root = $$;
        }
        ;

StructSpecifier : STRUCT OptTag LC DefList RC {
        ASTNode *stru = newKeyword(STRUCT);
        ASTNode *lc = newOperator(LC);
        ASTNode *rc = newOperator(RC);
        $$ = newASTNode("StructSpecifier", @1.first_line, 5, stru, $2, lc, $4, rc);
        }
        | STRUCT Tag {
        ASTNode *stru = newKeyword(STRUCT);
        $$ = newASTNode("StructSpecifier", @1.first_line, 2, stru, $2);
        }
        ;

OptTag : Tag {
        ASTNode *id = newID(yylval.string_val);
        $$ = newASTNode("OptTag", @1.first_line, 1, id);
        }
        | {
                $$=NULL;
        } 
        ;
Tag : ID {
        ASTNode *id = newID(yylval.string_val);
        $$ = newASTNode("Tag", @1.first_line, 1, id);
        };
/* Declarators */
VarDec: ID {
        ASTNode *id = newID(yylval.string_val);
        $$ = newASTNode("VarDec", @1.first_line, 1, id);
        }
        | VarDec LB INT RB {
        ASTNode *lb = newOperator(LB);
        ASTNode *int_ = newInt(yylval.int_val);
        ASTNode *rb = newOperator(RB);
        $$ = newASTNode("VarDec", @1.first_line, 4, $1, lb, int_, rb);
        }
        ;

FunDec: ID LP VarList RP {
        ASTNode *id = newID(yylval.string_val);
        ASTNode *lp = newOperator(LP);
        ASTNode *rp = newOperator(RP);
        $$ = newASTNode("FunDec", @1.first_line, 4, id, lp, $3, rp);
        }
        | ID LP RP {
        ASTNode *id = newID(yylval.string_val);
        ASTNode *lp = newOperator(LP);
        ASTNode *rp = newOperator(RP);
        $$ = newASTNode("FunDec", @1.first_line, 3, id, lp, rp);
        }
        ;

VarList: ParamDec COMMA VarList {
        ASTNode *comma = newASTNode("COMMA", -1, 0);
        $$ = newASTNode("VarList", @1.first_line, 3, $1, comma, $3);
        }
        | ParamDec {
        $$ = newASTNode("VarList", @1.first_line, 1, $1);
        }
        ;

ParamDec: Specifier VarDec{
        $$ = newASTNode("ParamDec", @1.first_line, 2, $1, $2);
        }
        ;

/* Statements */
CompSt: LC DefList StmtList RC {
        ASTNode *lc = newOperator(LC);
        ASTNode *rc = newOperator(RC);
        $$ = newASTNode("CompSt", @1.first_line, 4, lc, $2, $3, rc);
        }
        ;

StmtList: Stmt StmtList {
        $$ = newASTNode("StmtList", @1.first_line, 2, $1, $2);
        }
        | Epsilon
        ;

Stmt: Exp SEMI {
        ASTNode *semi = newSEMI();
        $$ = newASTNode("Stmt", @1.first_line, 2, $1, semi);
        }
        | CompSt {
        $$ = newASTNode("Stmt", @1.first_line, 1, $1);
        }
        | RETURN Exp SEMI {
        ASTNode *return_ = newKeyword(RETURN);
        ASTNode *semi = newSEMI();
        $$ = newASTNode("Stmt", @1.first_line, 3, return_, $2, semi);
        }
        | IF LP Exp RP Stmt {
        ASTNode *if_ = newKeyword(IF);
        ASTNode *lp = newOperator(LP);
        ASTNode *rp = newOperator(RP);
        $$ = newASTNode("Stmt", @1.first_line, 5, if_, lp, $3, rp, $5);
        }
        | IF LP Exp RP Stmt ELSE Stmt {
        ASTNode *if_ = newKeyword(IF);
        ASTNode *lp = newOperator(LP);
        ASTNode *rp = newOperator(RP);
        ASTNode *else_ = newKeyword(ELSE);
        $$ = newASTNode("Stmt", @1.first_line, 7, if_, lp, $3, rp, $5, else_, $7);
        }
        | WHILE LP Exp RP Stmt {
        ASTNode *while_ = newKeyword(WHILE);
        ASTNode *lp = newOperator(LP);
        ASTNode *rp = newOperator(RP);
        $$ = newASTNode("Stmt", @1.first_line, 5, while_, lp, $3, rp, $5);
        }
        ;

/* Local Definitions */
DefList: Def DefList {
        $$ = newASTNode("DefList", @1.first_line, 2, $1, $2);
        } | {
        $$ = NULL;
        };

Def: Specifier DecList SEMI{
        ASTNode *semi = newSEMI();
        $$ = newASTNode("Def", @1.first_line, 3, $1, $2, semi);
        ast_root = $$;
        };

DecList: Dec {
        $$ = newASTNode("DecList", @1.first_line, 1, $1);
        } | Dec COMMA DecList {
        ASTNode *comma = newCOMMA();
        $$ = newASTNode("DecList", @1.first_line, 3, $1, comma, $3);
        }; 

Dec: VarDec {
        $$ = newASTNode("Dec", @1.first_line, 1, $1);
        } | VarDec ASSIGNOP Exp {
        ASTNode *assignop = newOperator(ASSIGNOP);
        $$ = newASTNode("Dec", @1.first_line, 3, $1, assignop, $3);
        };

/* Expressions */
Exp: Exp ASSIGNOP Exp {
        ASTNode *assignop = newOperator(ASSIGNOP);
        breakpoint();
        $$ = newASTNode("Exp", @1.first_line, 3, $1, assignop, $3);
        }
        | Exp AND Exp {
        ASTNode *and_ = newOperator(AND);
        $$ = newASTNode("Exp", @1.first_line, 3, $1, and_, $3);
        }
        | Exp OR Exp {
        ASTNode *or_ = newOperator(OR);
        $$ = newASTNode("Exp", @1.first_line, 3, $1, or_, $3);        
        }
        | Exp RELOP Exp {
        ASTNode *relop = newOperator(RELOP);
        $$ = newASTNode("Exp", @1.first_line, 3, $1, relop, $3);
        }
        | Exp PLUS Exp {
        ASTNode *plus = newOperator(PLUS);
        $$ = newASTNode("Exp", @1.first_line, 3, $1, plus, $3);
        }
        | Exp MINUS Exp {
        ASTNode *minus = newOperator(MINUS);
        $$ = newASTNode("Exp", @1.first_line, 3, $1, minus, $3);
        }
        | Exp STAR Exp {
        ASTNode *star = newOperator(STAR);
        $$ = newASTNode("Exp", @1.first_line, 3, $1, star, $3); 
        }
        | Exp DIV Exp {
        ASTNode *div = newOperator(DIV);
        $$ = newASTNode("Exp", @1.first_line, 3, $1, div, $3); 
        }
        | LP Exp RP {
        ASTNode *lp = newOperator(RP);
        ASTNode *rp = newOperator(LP);
        $$ = newASTNode("Exp", @1.first_line, 3, lp, $2, rp); 
        }
        | MINUS Exp {
                Panic("Bad!");
                $$ = $2;
        }
        | NOT Exp{
        ASTNode *not_ = newOperator(NOT);
        $$ = newASTNode("Exp", @1.first_line, 2, NOT, $2); 
        }
        | ID LP Args RP {
        ASTNode *id = newID(yylval.string_val);
        ASTNode *lp = newOperator(LP);
        ASTNode *rp = newOperator(RP);
        $$ = newASTNode("Exp", @1.first_line, 4, id, lp, $3, rp);
        }
        | ID LP RP {
        ASTNode *id = newID(yylval.string_val);
        ASTNode *lp = newOperator(LP);
        ASTNode *rp = newOperator(RP);
        $$ = newASTNode("Exp", @1.first_line, 3, id, lp, rp);
        }
        | Exp LB Exp RB {
        ASTNode *lb = newOperator(LB);
        ASTNode *rb = newOperator(RB);
        $$ = newASTNode("Exp", @1.first_line, 4, $1, lb, $3, rb);
        }
        | Exp DOT ID {
        ASTNode *dot = newOperator(DOT);
        ASTNode *id = newID(yylval.string_val);
        $$ = newASTNode("Exp", @1.first_line, 3, $1, dot, id);
        }
        | ID { 
        Log("id matched!  %s", yylval.string_val);
        ASTNode *id = newID(yylval.string_val);
        $$ = newASTNode("Exp", @1.first_line, 1, id);
        }
        | INT {
        ASTNode *int_ = newInt(yylval.int_val);
        $$ = newASTNode("Exp", @1.first_line, 1, int_);
        }
        | FLOAT {
        ASTNode *float_ = newFloat(yylval.float_val);
        $$ = newASTNode("Exp", @1.first_line, 1, float_);
        }
        ;

Args: Exp COMMA Args {
        ASTNode *comma = newCOMMA();
        $$ = newASTNode("Args", @1.first_line, 3, $1, comma, $3);
        }
        | Exp {
        $$ = newASTNode("Args", @1.first_line, 1, $1);
        }
        ;

%%

void yyerror(char* s, ...) {
    //TODO: Handle error
    va_list ap;
    va_start(ap, s);
    fprintf(stderr, "TODO");
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
    Panic("Not implemented!\n");
}

void lyyerror(YYLTYPE t, char* s, ...) {
     va_list ap;
     va_start(ap, s);
     if(t.first_line) {
        fprintf(stderr, "%d.%d-%d.%d: error", t.first_line, t.first_column, t.last_line, t.last_column);
     }
     vfprintf(stderr, s, ap);
     fprintf(stderr, "\n");

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
    dfs_print(ast_root, 0);
}

void dfs_print(ASTNode *node, int depth) {
    if (node == NULL) {
        Panic("Null node!");
    }
    for(int i = 0; i < depth; i++) {
        printf("  "); // indentation
    }
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