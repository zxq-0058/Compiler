cd Code
flex -o ./lex.yy.c ./lexical.l
bison -o ./syntax.tab.c -d -v ./syntax.y
# -g调试信息
gcc -g -c ./syntax.tab.c -o ./syntax.tab.o
gcc -g -std=c99 -c -o main.o main.c
gcc -g -std=c99 -c -o semantics.o semantics.c
gcc -g -std=c99 -c -o syntax.tab.o syntax.tab.c
gcc -g -o parser ./main.o ./syntax.tab.o ./semantics.o -lfl -ly
