#ifndef _SYNTAX_H
#define _SYNTAX_H

#include <stdarg.h>
#include <stdlib.h>

#include "logger.h"
/**
 * A data structure for abstract syntax tree
 * TODO: All the struct below is under construction!
 */
typedef struct value {
    char id[32];
    char type[8];  // int or float
    unsigned int ival;
    float fval;
} value_t;

#define MAX_CHILD_N 32
typedef struct ASTNode {
    int type;    // to indicate a lexical or syntax element
    int lineno;  // line number for syntax unit
    const char *node_name;
    int child_num;
    struct ASTNode *child_list[MAX_CHILD_N];
    enum value_type { NONE, ID_, TYPE_, INT_, FLOAT_ } value_type;  // whether value is valid, -1 for invalid.
    value_t value;  //  some lexical token(ID、TYPE、INT、FLOAT)'s content
    enum exp_type {
        ASSIGN_EXP,  // Exp ASSIGNOP Exp
        BINARY_EXP,  // Exp AND Exp(etc.)
        P_EXP,       // LP Exp RP
        UNARY_EXP,   // MINUS Exp,  NOT Exp
        FUN_EXP,     // ID LP Args RP, ID LP RP
        ARR_EXP,     // Exp LB Exp RB
        STRU_EXP,    // Exp DOT ID
        ID_EXP,      // ID
        INT_EXP,     // INT
        FLOAT_EXP    //  FLOAT
    } exp_type;      // Type of expression
    enum {
        BI_PLUS,
        BI_MINUS,
        BI_STAR,
        BI_DIV,
        BI_AND,
        BI_OR,
        BI_RELOP,
    } bi_type;  // use for lab3, binary exp type
} ASTNode;

/* fundamental functions */
ASTNode *newASTNode(const char *name, int lineno, int numChildren, ...);
void add_child(ASTNode *parent, ASTNode *child);

/* global var ast_root will be initialized here*/
ASTNode *newProgram(int lineno, ASTNode *ExtDefList);

/* newASTNode functions for basic tokens, see syntax.y for more info */
ASTNode *newTYPE(const char *type);
ASTNode *newID(char *id_name);
ASTNode *newOperator(int op_type);
ASTNode *newInt(unsigned int _int);
ASTNode *newFloat(float _float);
ASTNode *newSEMI();
ASTNode *newCOMMA();
ASTNode *newKeyword(int key);

/* print the abstract syntax tree using dfs transverse */
void print_AST();
void dfs_print(ASTNode *node, int depth);

#endif