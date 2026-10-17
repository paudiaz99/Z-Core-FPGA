.section .text.start
.global _start

_start:
    lui sp, %hi(_stack_top)
    addi sp, sp, %lo(_stack_top)

    la t0, trap_entry
    csrw mtvec, t0

    la a0, __bss_start
    la a1, __bss_end
    j 2f
1:
    sw zero, 0(a0)
    addi a0, a0, 4
2:
    blt a0, a1, 1b

    call main

_loop:
    j _loop

    .align 2
    .global trap_entry
trap_entry:
    csrr a0, mcause
    csrr a1, mepc
    csrr a2, mtval
    call trap_report
3:
    j 3b

.section .bss
