.data
_prompt: .asciiz "Enter an integer:"
_ret: .asciiz "\n"
.globl main
.text
read:
  li $v0, 4
  la $a0, _prompt
  syscall
  li $v0, 5
  syscall
  jr $ra
write:
  li $v0, 1
  syscall
  li $v0, 4
  la $a0, _ret
  syscall
  move $v0, $0
  jr $ra
main:
  addi $sp, $sp, -4
  sw $fp 0($sp)
  move $fp, $sp
  addi $sp, $sp, -36
  addi $sp, $sp, -4
  sw $ra 0($sp)
  jal read
  lw $ra 0($sp)
  addi $sp, $sp, 4
  move $t0, $v0
  sw $t0, 0($fp)
  lw $t1 0($fp)
  sw $t1 4($fp)
  addi $sp, $sp, -4
  sw $ra 0($sp)
  jal read
  lw $ra 0($sp)
  addi $sp, $sp, 4
  move $t1, $v0
  sw $t1, 8($fp)
  lw $t2 8($fp)
  sw $t2 12($fp)
  lw $t2, 4($fp)
  lw $t3, 12($fp)
  sub $t4 $t2 $t3
  sw $t4, 16($fp)
  lw $t2 16($fp)
  sw $t2 20($fp)
  lw $t2, 4($fp)
  lw $t3, 12($fp)
  mul $t4 $t2 $t3
  sw $t4, 24($fp)
  lw $t2 24($fp)
  sw $t2 28($fp)
  lw $t2, 4($fp)
  lw $t3, 12($fp)
  div $t4 $t2 $t3
  sw $t4, 32($fp)
  lw $t2 32($fp)
  sw $t2 28($fp)
  lw $a0, 20($fp)
  addi $sp, $sp, -4
  sw $ra 0($sp)
  jal write
  lw $ra 0($sp)
  addi $sp, $sp, 4
  lw $a0, 28($fp)
  addi $sp, $sp, -4
  sw $ra 0($sp)
  jal write
  lw $ra 0($sp)
  addi $sp, $sp, 4
