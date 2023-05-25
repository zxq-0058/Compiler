# 实验四

(1)还是老规矩，先将整个大任务进行拆分：
- 将输入文件转化为结构体链表

（2）拆分称BasicBlock
首先我们需要知道怎么将中间代码拆分为基本块：
- 从第一条语句开始，直到遇到跳转语句，这些语句组成一个基本块
- 跳转语句的下一条语句也是一个基本块


```
FUNCTION main :
READ t1
v1 := t1
IF v1 >= #1 GOTO label1

GOTO label2

LABEL label1 :
WRITE #1
GOTO label3

LABEL label2 :
WRITE #0

LABEL label3 :
RETURN #0
```

（1）临时变量会不会跨越不同的基本块？
（2）局部优化时删除的无用代码的时，删除的应该是无效的临时变量，而不是Variable?