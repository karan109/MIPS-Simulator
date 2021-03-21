# A complete MIPS program
main:#main label
	addi $t0, $zero, 0      # add immediate
	addi $t1, $zero, 4		# add immediate
	add $t4, $t1, $t0		# add
	sub $t5, $t3, $t4		# subtract
# Empty Line :!%^
	store_loop:#store_loop label 
		addi $t0, $t0, 1#increment counter by 1
		sw $t0, -4($sp)
		addi $sp, $sp, -4
		bne $t0, $t1, store_loop
		# Empty Line
	load_loop:
		addi $t0, $t0, -1
		beq $t0, $zero, exit
		lw $t2, 0($sp)
		addi $sp, $sp, 4
		j load_loop
    # Empty Line # 4
	exit:
        addi $t0, $t0, 0x7fff
        addi $t1, $zero, -0x8000
        mul $t6, $t1, $t0
        slt $t3, $t1, $t0
        slt $t6, $t2, $t3