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
  sw $fp, 0($sp)
  move $fp, $sp
  addi $sp, $sp, -28
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal read
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  move $t0, $v0
  sw $t0, -4($fp)
  lw $t1, -4($fp)
  sw $t1, -8($fp)
  li $t1, 0
  sw $t1, -12($fp)
  lw $t1, -8($fp)
  li $t2, 0
  bne $t1, $t2, label1
j label3
label3:
  li $t1, 1
  li $t2, 0
  bne $t1, $t2, label1
j label2
label1:
  li $t1, 1
  sw $t1, -12($fp)
label2:
  lw $t1, -12($fp)
  sw $t1, -20($fp)
  lw $t1, -20($fp)
  li $t2, 0
  bne $t1, $t2, label4
j label5
label4:
  li $a0, 1
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal write
  lw $ra, 0($sp)
  addi $sp, $sp, 4
j label6
label5:
  li $t1, 0
  li $t2, 1
  sub $t3, $t1, $t2
  sw $t3, -24($fp)
  lw $a0, -24($fp)
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal write
  lw $ra, 0($sp)
  addi $sp, $sp, 4
label6:
  li $v0, 0
  move $sp, $fp
  lw $fp, 0($sp)
  addi $sp, $sp, 4
  jr $ra
