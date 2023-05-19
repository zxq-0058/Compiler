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
  addi $sp, $sp, -96
  addi $t0 $fp 8
  sw $t0 12($fp)
  lw $t0, 12($fp)
  li $t1, 0
  add $t2 $t0 $t1
  sw $t2, 16($fp)
  addi $sp, $sp, -4
  sw $ra 0($sp)
  jal read
  lw $ra 0($sp)
  addi $sp, $sp, 4
  move $t0, $v0
  sw $t0, 24($fp)
  addi $t1 16$(fp)
  lw $t1 24($fp)
  sw $t2 0($t1)
  addi $t1 $fp 8
  sw $t1 28($fp)
  lw $t1, 28($fp)
  li $t3, 4
  add $t4 $t1 $t3
  sw $t4, 32($fp)
  addi $sp, $sp, -4
  sw $ra 0($sp)
  jal read
  lw $ra 0($sp)
  addi $sp, $sp, 4
  move $t1, $v0
  sw $t1, 40($fp)
  addi $t3 32$(fp)
  lw $t3 40($fp)
  sw $t4 0($t3)
  addi $t3 $fp 8
  sw $t3 44($fp)
  lw $t3, 44($fp)
  li $t5, 8
  add $t6 $t3 $t5
  sw $t6, 48($fp)
  addi $t3 $fp 8
  sw $t3 56($fp)
  lw $t3, 56($fp)
  li $t5, 0
  add $t6 $t3 $t5
  sw $t6, 60($fp)
