#include <stdio.h>
#include <stdlib.h>

#include "logger.h"
#include "semantics.h"
extern ASTNode *ast_root;

extern void yyrestart(FILE *input_file);
extern int yyparse(void);
extern void print_AST();
extern int lexical_error;
extern int syntax_error;
// extern int yydebug;

int main(int argc, char **argv) {
    if (argc <= 1) {
        return 1;
    }
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    // yydebug = 1;
    yyparse();
    if (!lexical_error && !syntax_error) {
        // print_AST();
        program_handler(ast_root);
    }
    return 0;
}
