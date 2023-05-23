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
fact:
  addi $sp, $sp, -4
  sw $fp, 0($sp)
  move $fp, $sp
  addi $sp, $sp, -24
  lw $t0, 8($fp)
  sw $t0, -4($fp)
  lw $t0, -4($fp)
  li $t1, 1
  beq $t0, $t1, label1
j label2
label1:
  lw $v0, -4($fp)
  move $sp, $fp
  lw $fp, 0($sp)
  addi $sp, $sp, 4
  jr $ra
j label3
label2:
  lw $t0, -4($fp)
  li $t1, 1
  sub $t2, $t0, $t1
  sw $t2, -12($fp)
  lw $t0, -12($fp)
  addi $sp, $sp, -4
  sw $t0, 0($sp)
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal fact
  move $t0, $v0
  sw $t0, -16($fp)
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  lw $t0, -4($fp)
  lw $t1, -16($fp)
  mul $t2, $t0, $t1
  sw $t2, -20($fp)
  lw $v0, -20($fp)
  move $sp, $fp
  lw $fp, 0($sp)
  addi $sp, $sp, 4
  jr $ra
label3:
main:
  addi $sp, $sp, -4
  sw $fp, 0($sp)
  move $fp, $sp
  addi $sp, $sp, -20
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal read
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  move $t0, $v0
  sw $t0, -4($fp)
  lw $t1, -4($fp)
  sw $t1, -8($fp)
  lw $t1, -8($fp)
  li $t2, 1
  bgt $t1, $t2, label4
j label5
label4:
  lw $t1, -8($fp)
  addi $sp, $sp, -4
  sw $t1, 0($sp)
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal fact
  move $t1, $v0
  sw $t1, -12($fp)
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  lw $t1, -12($fp)
  sw $t1, -16($fp)
j label6
label5:
  li $t1, 1
  sw $t1, -16($fp)
label6:
  lw $a0, -16($fp)
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
