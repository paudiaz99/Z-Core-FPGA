.section .text.start
.global _start

_start:
    # Initialize stack pointer to top of RAM
    # Stack at 4KB (0x00001000)
    lui sp, %hi(_stack_top)
    addi sp, sp, %lo(_stack_top)
    
    # Clear BSS section
    la a0, __bss_start
    la a1, __bss_end
    j 2f
1:
    sw zero, 0(a0)
    addi a0, a0, 4
2:
    blt a0, a1, 1b
    
    # Call main function
    call main
    
    # If main returns, loop forever
_loop:
    j _loop

.section .bss
