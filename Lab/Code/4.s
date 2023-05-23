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
print:
  addi $sp, $sp, -4
  sw $fp, 0($sp)
  move $fp, $sp
  addi $sp, $sp, -12
  lw $t0, 8($fp)
  sw $t0, -4($fp)
  lw $t0, 12($fp)
  sw $t0, -8($fp)
  lw $a0, -4($fp)
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal write
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  lw $a0, -8($fp)
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal write
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  li $v0, 0
  move $sp, $fp
  lw $fp, 0($sp)
  addi $sp, $sp, 4
  jr $ra
main:
  addi $sp, $sp, -4
  sw $fp, 0($sp)
  move $fp, $sp
  addi $sp, $sp, -16
  li $t0, 2
  addi $sp, $sp, -4
  sw $t0, 0($sp)
  li $t0, 1
  addi $sp, $sp, -4
  sw $t0, 0($sp)
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal print
  move $t0, $v0
  sw $t0, -12($fp)
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  li $v0, 0
  move $sp, $fp
  lw $fp, 0($sp)
  addi $sp, $sp, 4
  jr $ra
