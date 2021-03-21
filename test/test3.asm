# Execution is not interrupted when the same register as sw is encountered
addi $s0, $zero, 1000
addi $t0, $t0, 5
sw $t0, 0($s0)
loop:
	addi $t1, $t1, 6
	addi $t2, $t2, 1
	bne $t2, $t0, loop
sw $t3, 1032($s0)
addi $t5, $t5, -3
addi $t3, $t3, -1000
