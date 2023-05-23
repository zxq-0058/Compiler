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
f:
  addi $sp, $sp, -4
  sw $fp, 0($sp)
  move $fp, $sp
  addi $sp, $sp, -8
  lw $t0, 8($fp)
  sw $t0, -4($fp)
  li $v0, 0
  move $sp, $fp
  lw $fp, 0($sp)
  addi $sp, $sp, 4
  jr $ra
main:
  addi $sp, $sp, -4
  sw $fp, 0($sp)
  move $fp, $sp
  addi $sp, $sp, -12
  li $t0, 0
  sw $t0, -4($fp)
