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
  li $a0, 255
  addi $sp, $sp, -4
  sw $ra 0($sp)
  jal write
  lw $ra 0($sp)
  addi $sp, $sp, 4
  move $v0, $0
  jr $ra
f:
