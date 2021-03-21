# Execution is not interrupted when the register of lw is not encountered
addi $s0, $zero, 1000
addi $t0, $t0, 5
lw $t5, 0($s0)
loop:
	addi $t1, $t1, 6
	addi $t2, $t2, 1
	bne $t2, $t0, loop

