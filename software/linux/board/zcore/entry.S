/* SPDX-License-Identifier: MIT
 * Existing ROM calls entry as void(void); Linux needs a0=hartid, a1=DTB.
 * Upload only after reset. ROM evicts D-cache and drains its write buffer.
 */
.section .text.entry,"ax",@progbits
.global _start
_start:
    csrw mie, zero
    csrci mstatus, 8
    li t0, 0x04002008
    sw zero, 0(t0)
    fence rw, rw
    fence.i
    li a0, 0
    li a1, 0x13ff0000
    li t0, 0x10000000
    jr t0
