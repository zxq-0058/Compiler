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
  addi $sp, $sp, -92
  li $t0, 1
  sw $t0, -44($fp)
  addi $t0, $fp, -40
  sw $t0, -48($fp)
  lw $t0, -48($fp)
  li $t1, 20
  add $t2, $t0, $t1
  sw $t2, -52($fp)
  lw $t0, -52($fp)
  lw $t0, 0($t0)
  sw $t0, -56($fp)
  li $t0, 3
  li $t1, 2
  mul $t2, $t0, $t1
  sw $t2, -60($fp)
  lw $t0, -44($fp)
  lw $t1, -60($fp)
  add $t2, $t0, $t1
  sw $t2, -64($fp)
  li $t0, 0
  sw $t0, -68($fp)
  lw $t0, -44($fp)
  li $t1, 0
  bne $t0, $t1, label1
j label3
label3:
  addi $t0, $fp, -40
  sw $t0, -72($fp)
  lw $t0, -72($fp)
  li $t1, 8
  add $t2, $t0, $t1
  sw $t2, -76($fp)
  lw $t0, -76($fp)
  lw $t0, 0($t0)
  sw $t0, -80($fp)
  lw $t0, -80($fp)
  li $t1, 9
  add $t2, $t0, $t1
  sw $t2, -84($fp)
  lw $t0, -84($fp)
  li $t1, 0
  bne $t0, $t1, label1
j label2
label1:
  li $t0, 1
  sw $t0, -68($fp)
label2:
  li $t0, 0
  sw $t0, -88($fp)
  li $t0, 2
  li $t1, 1
  bgt $t0, $t1, label4
j label5
label4:
  li $t0, 1
  sw $t0, -88($fp)
label5:
  lw $a0, -88($fp)
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
