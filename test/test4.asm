# Execution is interrupted when the register of lw is encountered
addi $s0, $zero, 1000
addi $t0, $t0, 2
lw $t5, 0($s0)
loop:
	addi $t1, $t1, 6
	addi $t2, $t2, 1
	bne $t2, $t0, loop
addi $t5, $t5, -10

