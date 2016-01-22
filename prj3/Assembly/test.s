addi $a1, $zero, 10

loop: addi $a0, $a0, 1
    sw $a0, 0($a0)
    beq $a0, $a1, done
    beq $zero, $zero, loop

done: addi $s0, $zero, 10