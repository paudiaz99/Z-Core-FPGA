    .section .text.start
    .global _start
_start:
    lui  sp, %hi(_stack_top)
    addi sp, sp, %lo(_stack_top)

    la   a0, __bss_start
    la   a1, __bss_end
    bgeu a0, a1, 2f
1:
    sw   zero, 0(a0)
    addi a0, a0, 4
    bltu a0, a1, 1b
2:
    li   a0, 0
    li   a1, 0
    call main

_halt:
    j _halt
