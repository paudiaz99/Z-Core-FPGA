.section .text.start
.global _start

_start:
    lui sp, %hi(_stack_top)
    addi sp, sp, %lo(_stack_top)

    /* Install trap vector (direct mode) so an unhandled exception in the
       uploaded app reports its cause instead of silently jumping to 0x0
       (mtvec resets to 0 = bootloader entry, which looked like a "reboot"). */
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

/* Trap entry: report cause/epc/tval and halt (no mret). Registers may be
   clobbered freely since we never return to the faulting context. */
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
