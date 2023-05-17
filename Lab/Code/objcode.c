#include "objcode.h"

#include "intercode.h"

FILE *spm_code_file = NULL;
extern InterCodes head;
extern char *print_operand(Operand op);

extern int var_count;
extern int tmp_count;

reg_t registers[NUM_REGISTERS];

/// @brief 初始化寄存器（t%d）寄存器
void init_registers() {
    // 初始化寄存器
    for (int i = 0; i < NUM_REGISTERS; i++) {
        sprintf(registers[i].name, "$t%d", i);
        registers[i].is_free = 1;  // 初始时所有寄存器都可用
    }
}

/// @brief 获得一个寄存器
reg_t *get_reg(Operand op) {
    // 需要先判断op
    for (int i = 0; i < NUM_REGISTERS; i++) {
        if (registers[i].is_free) {
            registers[i].is_free = 0;  // 标记为不可用
            return &registers[i];
        }
    }
    // 朴素寄存器实现中，这里不应该被执行到
    Panic("Should not reach here in naive register allocation.");
}

int get_offset(Operand op) {
    if (op->kind == OP_VARIABLE) {
        Panic("Not implemented yet");
        // return op->u.var_no * 4;
    } else if (op->kind == OP_TEMP) {
        Panic("Not implemented yet");
        // return (op->u.temp_no + var_count) * 4;
    } else {
        Panic("Invalid Operand");
    }
}

/// @brief 实验中的mips的寄存器(for debugging)
const char *reg_names[] = {
    "$zero",                       // 常数0
    "$at",                         // 保留给汇编器
    "$v0",   "$v1",                // 函数返回值或者表达式求值(在实验中只会用到v0,v1另作他用)
    "$a0",   "$a1", "$a2", "$a3",  // 函数参数（跨函数不保留）
    "$t0",   "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",  // 临时变量（跨函数不保留）
    "$s0",   "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",  // 保存变量（跨函数不保留）
    "$t8",   "$t9",  // 临时变量， 函数调用者负责保存（跨函数不保留）
    "$k0",   "$k1",  // 保留给操作系统（系统调用）
    "$gp",           // 全局指针
    "$sp",           // 栈指针
    "$fp",           // 帧指针
    "$ra"            // 返回地址
};

/// @brief 初始化目标代码（实际上就是将讲义上的固定代码写入spm_code_file）
void obj_init() {
    const char *str =
        ".data\n"
        "_prompt: .asciiz \"Enter an integer:\"\n"
        "_ret: .asciiz \"\\n\"\n"
        ".globl main\n"
        ".text\n"
        "read:\n"
        "  li $v0, 4\n"
        "  la $a0, _prompt\n"
        "  syscall\n"
        "  li $v0, 5\n"
        "  syscall\n"
        "  jr $ra\n"
        "write:\n"
        "  li $v0, 1\n"
        "  syscall\n"
        "  li $v0, 4\n"
        "  la $a0, _ret\n"
        "  syscall\n"
        "  move $v0, $0\n"
        "  jr $ra\n";
    fprintf(spm_code_file, "%s", str);
}

/// @brief 将中间代码转换为目标代码
void ir2obj(FILE *fp) {
    spm_code_file = fp;
    obj_init();
    InterCodes p = head;
    while (p != NULL) {
        switch (p->code->kind) {
            case IR_LABEL:
                fprintf(spm_code_file, "%s:\n", print_operand(p->code->u.one.op));
                break;
            case IR_FUNCTION:
                fun_obj(p->code->u.one.op->u.func_name);
                break;
            case IR_ASSIGN:
                Panic("Not implemented yet");
                break;
            case IR_ADD:
                Panic("Not implemented yet");
            case IR_ADDRADD:
                Panic("Not implemented yet");
                break;
            case IR_SUB:
                Panic("Not implemented yet");
                break;
            case IR_MUL:
                Panic("Not implemented yet");
                break;
            case IR_DIV:
                Panic("Not implemented yet");
                break;
            case IR_GOTO:
                fprintf(spm_code_file, "j %s\n", print_operand(p->code->u.one.op));
                break;
            case IR_IFGOTO:
                Panic("Not implemented yet");
                break;
            case IR_RETURN:
                Panic("Not implemented yet");
                break;
            case IR_DEC:
                Panic("Not implemented yet");
                break;
            case IR_ARG:
                Panic("Not implemented yet");
                break;
            case IR_CALL:
                Panic("Not implemented yet");
                break;
            case IR_PARAM:
                Panic("Not implemented yet");
                break;
            case IR_READ:
                Panic("Not implemented yet");
                break;
            case IR_WRITE:
                write_obj(p->code->u.one.op);
                break;
            default:
                Panic("Invalid InterCode");
                break;
        }
        p = p->next;
    }
}

/// @brief 中间代码为函数声明
static void inline fun_obj(char *fun_name) { fprintf(spm_code_file, "%s:\n", fun_name); }

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
        "  addi $sp, $sp, 4\n";  // 恢复栈指针
    fprintf(spm_code_file, "%s", str);
}

/// @brief 参数压入栈，调用read，随后返回
/// @param op 参数OP
static void inline read_obj(Operand op) {
    const char *str =
        "  addi $sp, $sp, -4\n"  // 为返回地址分配空间
        "  sw $ra 0($sp)\n"      // 保存返回地址
        "  jal read\n"           // 调用read函数
        "  lw $ra 0($sp)\n"      // 恢复返回地址
        "  addi $sp, $sp, 4\n";  // 恢复栈指针
    fprintf(spm_code_file, "%s", str);

    // 还没想好要怎么把返回值传递给我的op
    Panic("Not implemented yet");
}

static void sub_obj(Operand result, Operand op1, Operand op2) {
    reg_t *reg_op1 = get_reg();
    fprintf(" lw %s, %d($fp)\n", reg_op1->name, get_offset(op));
    reg_t *reg_op2 = get_reg();
    reg_t *reg_result = get_reg(result);

    fprintf(" sw %s, %d($fp)\n", reg_result->name, get_offset(result));
    fprintf(spm_code_file, "  sub %s, %s, %s\n", reg_result->name, reg_op1->name, reg_op2->name);
}