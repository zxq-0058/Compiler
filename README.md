# Compiler
Compiler for my Homework Labs

从零开始实现C - -编译器，包括词法分析、语法分析、语义分析、中间代码生成和优化、目标代码生成等阶段。

- 词法分析、语法分析(Lab1)：使用Flex和Bison 实现词法 + 语法分析器，生成语法分析树：\href{https://github.com/zxq-0058/Compiler/blob/Lab5/Lab/Code/syntax.y}{\textit{code link}}
- 语义分析：在语法树的基础上构造作用域和符号表，检查同作用域内变量重复等语义错误，并生成中间代码： \href{https://github.com/zxq-0058/Compiler/blob/Lab5/Lab/Code/semantics.c}{\textit{code link}}
- 中间代码优化：包括局部代码优化和全局优化，局部代码优化基于DAG进行局部公共子表达式的消除、局部常量折叠；全局优化基于数据流分析实现到达定值分析和循环不变式外提：\href{https://github.com/zxq-0058/Compiler/blob/Lab5/Lab/Code/basicblock.c}{\textit{code link}}
- 目标代码生成：基于中间代码生成mips汇编代码：\href{https://github.com/zxq-0058/Compiler/blob/Lab5/Lab/Code/objcode.c}{\textit{code link}}


在branch下可以进行多个实验分支之间的切换，每个实验分支下都有对应的测试目录。
