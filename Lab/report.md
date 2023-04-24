# 实验三

(1)小任务1，对于
```
int main() {
    return 1;
}
```
能够生成：
```
FUNCTION main :
```

(2)小任务2，对于
```
int i;
```
将其与一个v%d绑定（vardec处理函数）

(3) 任务3，对于
```
int i = 0;
```
将其与一个v%d绑定（vardec处理函数）并且进行三地址码的生成
```
v1 = #0
```


(4) 任务4，对于
```
int main() {
    int i = 1, j = 2;
    int k = i;
    return 1;
}
```

```
FUNCTION main :
t0 := #1
v0 := t0
t1 := #2
v1 := t1
t2 := v0
v2 := t2

```


(5) 任务5，处理函数参数和返回值的问题
```
FUNCTION main :
PARAM v0
PARAM v1
```

(6)处理函数简单的计算问题
Exp_函数(四则运算)

(7)处理条件分支的问题
GOTO和IF(x)relop(y) GOTO z



结构体作为函数参数传递时，传进来的是一个指针

结构体：
（1）结构体定义：预先存储一些必要的信息，memSize（已经完成）
（2）结构体的使用：
- 声明一个结构体变量：DEC [size] 这个比较简单，memSize
- 使用结构体的域的时候：find_field（struct, name），域的偏移量计算出来
    Exp.id:
    (1) tmp.o1
    解析到tmp, 

    翻译模式
    ```
    exp(exp, id, place)
        t1 = new_tmp();
        exp(exp, t1)  t1最后会是一个地址变量
        offset = find_field(struct, name);
        addr := t1 + offset;
        place := *addr; // insert_assign_ir(ASS_GETVAL)
    ```

    结构体变量作为函数参数，它是一个指针operand(Vardec需要修改)
    结构体变量作为局部变量，它是一个变量型的Operand

## 问题合集


变量和临时变量的区别：
- 变量是在符号表中的（原先代码就有的）
- 临时变量则是因为使用三地址码出现的

(1)声明一个变量(Vardec函数？？)
- 不赋予初值
那么就直接加入符号表（每一个变量都会与一个var_conut进行唯一绑定）
- 赋予初值
    - 常量 int n = 0;
        - 加入符号表（与var_count绑定）
    - 来自其它变量
        int n = i;
        - 加入符号表（与var_count绑定）
        - 生成三地址码
        ```
        t1 = i
        n = t1        
        ```

(2)表达式

(2.1)ASSIGN
- 两个操作数
    - 常量
    - 变量
    - 临时变量
- 生成三地址码
    ```
    t1 = i
    n = t1
    ```
    


最小化测试单元：
遇到错误尽可能精确定位错误，设计最小化测试单元进行测试