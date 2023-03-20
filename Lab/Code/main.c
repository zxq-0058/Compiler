#include <stdio.h>
#include <stdlib.h>
#include "logger.h"
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
    print_AST(); // TODO: error handle
    return 0;
}

// #include <stdio.h>
// #include <stdlib.h>

// extern FILE* yyin;
// extern int yylex();

// int main(int argc, char** argv) {
//     if(argc > 1) {
//         if (!(yyin = fopen(argv[1], "r"))) {
//             perror(argv[1]);
//             exit(1);
//         }
//     }
//     while (yylex() != 0);
//     return 0;
// }
