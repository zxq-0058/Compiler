cd Code
flex -o ./lex.yy.c ./lexical.l
bison -o ./syntax.tab.c -d -v ./syntax.y
# -g调试信息
gcc -g -std=c99   -c -o intercode.o intercode.c
gcc -g -c ./syntax.tab.c -o ./syntax.tab.o
gcc -g -std=c99 -c -o main.o main.c
gcc -g -std=c99 -c -o semantics.o semantics.c
gcc -g -std=c99 -c -o syntax.tab.o syntax.tab.c
gcc -g -std=c99 -c -o spimcode.o spimcode.c
gcc -g -o parser ./intercode.o ./main.o ./syntax.tab.o ./semantics.o spimcode.o -lfl -ly
