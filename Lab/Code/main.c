#include <stdio.h>
#include <stdlib.h>

#include "intercode.h"
#include "logger.h"
#include "objcode.h"
#include "semantics.h"

extern ASTNode *ast_root;

extern void yyrestart(FILE *input_file);
extern int yyparse(void);
extern void print_AST();
extern void program_handler(ASTNode *root);
extern void translate_program(ASTNode *root, FILE *out);
extern void ir2obj(FILE *fp);
extern void optimize_intercodes(FILE *in, FILE *out);
extern void print_file_content(FILE *file);
extern int lexical_error;
extern int syntax_error;
// extern int yydebug;

#define LAB_5

int main(int argc, char **argv) {
#ifndef LAB_5
    if (argc <= 2) {
        fprintf(stderr, "Usage: %s <input> <output>）\n", argv[0]);
        return 1;
    }
    FILE *in = fopen(argv[1], "r");
    FILE *ir = fopen("intercode.ir", "w+");
    FILE *out = fopen(argv[2], "w");
    if (!in || !out) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(in);
    // yydebug = 1;
    yyparse();
    if (!lexical_error && !syntax_error) {
        // print_AST();
        program_handler(ast_root);
        translate_program(ast_root, ir);
        fflush(ir);

        setlinebuf(out);
        optimize_intercodes(ir, out);
    }
    return 0;
#else
    if (argc <= 2) {
        fprintf(stderr, "Usage: %s <input> <output>）\n", argv[0]);
        return 1;
    }
    FILE *in = fopen(argv[1], "r");   // 输入中间代码文件
    FILE *out = fopen(argv[2], "w");  // 输出优化后的中间代码文件
    if (!in || !out) {
        perror(argv[1]);
        return 1;
    }
    optimize_intercodes(in, out);
    return 0;
#endif
}
