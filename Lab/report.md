# 实验四


实话实话，我觉得应该先把样例的汇编代码看懂……

一开始上来先不要考虑函数调用的怎么写，我们先考虑只有一个函数的情况。




（1）小任务一(打印常数)：
int main() {
    write(1);
}

生成代码

```c
/// @brief 参数压入栈，调用write，随后返回
/// @param op 参数OP
static void inline write_obj(Operand op) {
    if (op->kind == OP_CONSTANT) {
        fprintf(spm_code_file, "  li $a0, %d\n", op->u.value);
    } else {
        Panic("Not implemented yet");
    }
    const char *str =
        "  addi $sp, $sp, -4\n"  // 为返回地址分配空间
        "  sw $ra 0($sp)\n"      // 保存返回地址
        "  jal write\n"          // 调用write函数
        "  lw $ra 0($sp)\n"      // 恢复返回地址
        "  addi $sp, $sp, 4\n"   // 恢复栈指针
        "  move $v0, $0\n"       // 将函数的返回值设置为零
        "  jr $ra\n";            // 返回
    fprintf(spm_code_file, "%s", str);
}
```

从这个例子可以看出，代码风格非常非常重要，写出赏心悦目的代码并不是一件简单的事情

（2）小任务二：
int main() {
    write(read());
}



