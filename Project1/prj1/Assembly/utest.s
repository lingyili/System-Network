//addi $t0, $zero, 5
//addi $t1, $zero, 5
//beq $t0, $t1, A
//addi $t1, $zero, 3
//A: addi $t0, $zero, 2
//    add $t0, $t1, $t0
//    nand $t0, $t0, $t1
//sw $t0, 0x000e($zero)
//addi $t1, $zero, 1
//lw $t1, 0x000e($zero)
//addi $s0, $zero, 0x0001
//jalr $s0, $at
//end: halt

addi $t0, $zero, 1
addi $t1, $zero, 2
loop: addi $t0, $t0, 1
beq $t0, $t1, loop
end: addi $t0, $t0, 10
