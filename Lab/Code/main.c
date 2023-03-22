#include <stdio.h>
#include <stdlib.h>
#include "logger.h"

extern void yyrestart(FILE *input_file);
extern int yyparse(void);
extern void print_AST();
// extern int yydebug;

int main(int argc, char **argv)
{
    if (argc <= 1)
    {
        return 1;
    }
    FILE *f = fopen(argv[1], "r");
    if (!f)
    {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    // yydebug = 1;
    yyparse();
    print_AST();
    return 0;
}

