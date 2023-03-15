%option noyywrap nodefault yylineno
%{
    #include <stdio.h>
    #include <stdlib.h>
    #include "syntax.tab.h"
    #include "logger.h"

    extern YYSTYPE yylval;

    #define LINE_SIZE (512)
    char linebuf[LINE_SIZE];   /* line buffer */

    /* To record location info for bison */
    int yycolumn = 1;
    extern YYLTYPE yylloc;
    #define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

    /* Indicate whether lexical errors occur */
    int lexical_error = 0;
%}

%{
    /* Declare any global variables or define any macros here */
%}
%x  COMMENT
/* Define regular expressions for each token */
/* TODO:: add you regular expression here! */
STRUCT struct
RETURN return 
IF if
ELSE else 
WHILE while
SEMI ;
COMMA ,
ASSIGNOP =
RELOP >|<|>=|<=|==|!=
PLUS \+
MINUS \-
STAR \*
DIV \/
AND &&
OR \|\|
DOT \.
NOT \!
TYPE int|float
LP \(
RP \)
LB \[
RB \]
LC \{
RC \}
/* 将正整数分为三种类型考虑 */
HEX 0[xX][0-9a-fA-F]+
OCTONARY 0[0-7]*
DECIMAL 0|[1-9][0-9]*
/* 这个是从C的float定义抄过来的，不是讲义的描述内容，所以待定检查*/
EXP_PART [e|E][-+]?[0-9]+
FLOAT_1 [0-9]+\.[0-9]+
FLOAT_2 [0-9]+\.[0-9]+{EXP_PART}
FLOAT_3 [0-9]+\.{EXP_PART}
FLOAT_4 \.[0-9]+{EXP_PART}
FLOAT {FLOAT_1}|{FLOAT_2}|{FLOAT_3}|{FLOAT_4}

ID [a-zA-Z_][a-zA-Z0-9_]*
WHITE_SPACE    [ \t\f\r\v]+

%%

{STRUCT}   { yylval.ast_node = newKeyword(STRUCT); return STRUCT;}
{RETURN}   { yylval.ast_node = newKeyword(RETURN); return RETURN;}
{IF}       { yylval.ast_node = newKeyword(IF); return IF;}
{ELSE}     { yylval.ast_node = newKeyword(ELSE); return ELSE;}
{WHILE}    { yylval.ast_node = newKeyword(WHILE); return WHILE;}
{SEMI}     { yylval.ast_node = newSEMI(); return SEMI;}
{COMMA}    { yylval.ast_node = newCOMMA(); return COMMA;}
{ASSIGNOP} { yylval.ast_node = newOperator(ASSIGNOP); return ASSIGNOP;}
{RELOP}    { yylval.ast_node = newOperator(RELOP); return RELOP;}
{PLUS}     { yylval.ast_node = newOperator(PLUS); return PLUS;}
{MINUS}    { yylval.ast_node = newOperator(MINUS); return MINUS;}
{STAR}     { yylval.ast_node = newOperator(STAR); return STAR;}
{DIV}      { yylval.ast_node = newOperator(DIV); return DIV;}
{AND}      { yylval.ast_node = newOperator(AND); return AND;}
{OR}       { yylval.ast_node = newOperator(OR); return OR;}

{FLOAT} { 
        float float_val = (float)atof(yytext);
        yylval.ast_node = newFloat(float_val);
        Log("Float numer %f", float_val);
        return FLOAT;
}

{DOT}      { yylval.ast_node = newOperator(DOT); return DOT;}
{NOT}      { yylval.ast_node = newOperator(NOT); return NOT;}

{TYPE}     { 
        yylval.ast_node = newTYPE(yytext);
        return TYPE;
}

{LP}       { yylval.ast_node = newOperator(LP); return LP;}
{RP}       { yylval.ast_node = newOperator(RP); return RP;}
{LB}       { yylval.ast_node = newOperator(LB); return LB;}
{RB}       { yylval.ast_node = newOperator(RB); return RB;}
{LC}       { yylval.ast_node = newOperator(LC); return LC;}
{RC}       { yylval.ast_node = newOperator(RC); return RC;}

{HEX}      {
        char * end_ptr;
        char hex_str[32];
        strcpy(hex_str, yytext);
        int int_val = (int)strtol(hex_str, &end_ptr, 16);
        if(end_ptr != hex_str + strlen(hex_str)) Panic("Invaild hex number!");
        yylval.ast_node = newInt(int_val);
        return INT;
}

{OCTONARY} {
        char * end_ptr;
        char oct_str[32];
        strcpy(oct_str, yytext);
        int int_val = (int)strtol(oct_str, &end_ptr, 8);
        if(end_ptr != oct_str + strlen(oct_str)) Panic("Invaild octonary number!");
        yylval.ast_node = newInt(int_val);
        return INT;
}

{DECIMAL} {
        int int_val = atoi(yytext);
        yylval.ast_node = newInt(int_val);
        return INT;
}

{ID}       { 
    yylval.ast_node = newID(yytext);
    return ID;
}

"//"       {
    char c = input();
    while(c != '\n')c = input();
}

"/*"   {BEGIN(COMMENT);}
<COMMENT>"*/" {BEGIN(INITIAL);}
<COMMENT>([^*]|\n)+|.
<COMMENT><<EOF>> {
    printf("Error type A at Line %d: EOF in comment\n", yylineno);
    lexical_error = 1;
    BEGIN(INITIAL);
}

\n {
    yycolumn = 1;
}

{WHITE_SPACE} { /* Does Nothing */}
. { 
    printf("Error type A at Line %d: Mysterious character \"%s\".\n", yylineno, yytext);
    lexical_error = 1;
}

%%
