    .equ UART_BASE, 0x04000000
    .equ UART_TX,   0x00
    .equ UART_STAT, 0x08

    .equ SDRAM_BASE,   0x10000000
    .equ SCRATCH_ADDR, 0x10800000

    .section .text.start
    .global _start
_start:

    j _real_start

putc:
    li   t0, UART_BASE
.Lputc_wait:
    lw   t1, UART_STAT(t0)
    andi t1, t1, 1
    beqz t1, .Lputc_wait
    sw   a0, UART_TX(t0)
.Lputc_wait2:
    lw   t1, UART_STAT(t0)
    andi t1, t1, 1
    beqz t1, .Lputc_wait2
    ret

puthex32:
    addi sp, sp, -4
    sw   ra, 0(sp)
    li   t2, 28
.Lhex_loop:
    srl  t3, s1, t2
    andi t3, t3, 0xF
    li   a0, '0'
    add  a0, a0, t3
    li   t1, '9'
    ble  a0, t1, .Lhex_ok
    addi a0, a0, 7
.Lhex_ok:
    call putc
    addi t2, t2, -4
    bge  t2, zero, .Lhex_loop
    lw   ra, 0(sp)
    addi sp, sp, 4
    ret

print_crlf:
    addi sp, sp, -4
    sw   ra, 0(sp)
    li   a0, '\r'
    call putc
    li   a0, '\n'
    call putc
    lw   ra, 0(sp)
    addi sp, sp, 4
    ret

_real_start:
    li   sp, 0x10900000

    li   a0, 'S'
    call putc
    li   a0, '>'
    call putc
    call print_crlf

    li   a0, 'L'
    call putc
    li   a0, 'W'
    call putc
    li   a0, ':'
    call putc

    li   s0, SDRAM_BASE
    li   s2, 8
.Llw_loop:
    li   a0, ' '
    call putc
    lw   s1, 0(s0)
    call puthex32
    addi s0, s0, 4
    addi s2, s2, -1
    bnez s2, .Llw_loop
    call print_crlf

    li   a0, 'W'
    call putc
    li   a0, 'R'
    call putc
    li   a0, ':'
    call putc
    li   a0, ' '
    call putc

    li   s0, SCRATCH_ADDR
    li   s1, 0xDEADBEEF
    lui  s1, 0xDEADB
    addi s1, s1, -273
    lui  s1, 0xDEADE
    addi s1, s1, -0x111
    /* Just use the correct encoding: */
    li   s1, 0xDEADBEEF
    call puthex32
    sw   s1, 0(s0)

    li   a0, ' '
    call putc
    li   a0, 'R'
    call putc
    li   a0, 'B'
    call putc
    li   a0, ':'
    call putc
    li   a0, ' '
    call putc

    lw   s1, 0(s0)
    call puthex32
    call print_crlf

    li   s0, SCRATCH_ADDR
    li   s1, 0x44332211
    sw   s1, 0(s0)

    li   s2, 0
.Lbyte_loop:
    li   a0, 'B'
    call putc
    li   a0, '0'
    add  a0, a0, s2
    call putc
    li   a0, ':'
    call putc

    add  t3, s0, s2
    lbu  s1, 0(t3)
    srli t3, s1, 4
    andi t3, t3, 0xF
    li   a0, '0'
    add  a0, a0, t3
    li   t1, '9'
    ble  a0, t1, .Lb_hi_ok
    addi a0, a0, 7
.Lb_hi_ok:
    call putc
    andi t3, s1, 0xF
    li   a0, '0'
    add  a0, a0, t3
    li   t1, '9'
    ble  a0, t1, .Lb_lo_ok
    addi a0, a0, 7
.Lb_lo_ok:
    call putc
    li   a0, ' '
    call putc

    addi s2, s2, 1
    li   t1, 4
    blt  s2, t1, .Lbyte_loop
    call print_crlf

    li   a0, 'C'
    call putc
    li   a0, 'B'
    call putc
    li   a0, ':'
    call putc

    li   s0, SDRAM_BASE
    li   s2, 8
.Lcb_loop:
    li   a0, ' '
    call putc
    lbu  s1, 0(s0)
    srli t3, s1, 4
    andi t3, t3, 0xF
    li   a0, '0'
    add  a0, a0, t3
    li   t1, '9'
    ble  a0, t1, .Lcb_hi_ok
    addi a0, a0, 7
.Lcb_hi_ok:
    call putc
    andi t3, s1, 0xF
    li   a0, '0'
    add  a0, a0, t3
    li   t1, '9'
    ble  a0, t1, .Lcb_lo_ok
    addi a0, a0, 7
.Lcb_lo_ok:
    call putc

    addi s0, s0, 1
    addi s2, s2, -1
    bnez s2, .Lcb_loop
    call print_crlf

    li   a0, 'F'
    call putc
    li   a0, 'W'
    call putc
    li   a0, ':'
    call putc
    li   a0, ' '
    call putc

    li   s0, SCRATCH_ADDR
    addi s0, s0, 16
    li   s1, 0xAABBCCDD
    sw   s1, 0(s0)
    lw   s1, 0(s0)
    call puthex32

    li   a0, ' '
    call putc
    li   a0, 'B'
    call putc
    li   a0, ':'
    call putc

    li   s2, 0
.Lfull_byte:
    li   a0, ' '
    call putc
    add  t3, s0, s2
    lbu  s1, 0(t3)
    srli t3, s1, 4
    andi t3, t3, 0xF
    li   a0, '0'
    add  a0, a0, t3
    li   t1, '9'
    ble  a0, t1, .Lfb_hi_ok
    addi a0, a0, 7
.Lfb_hi_ok:
    call putc
    andi t3, s1, 0xF
    li   a0, '0'
    add  a0, a0, t3
    li   t1, '9'
    ble  a0, t1, .Lfb_lo_ok
    addi a0, a0, 7
.Lfb_lo_ok:
    call putc
    addi s2, s2, 1
    li   t1, 4
    blt  s2, t1, .Lfull_byte
    call print_crlf

    li   a0, 'O'
    call putc
    li   a0, 'K'
    call putc
    call print_crlf

.global _exit
_exit:
    j _exit
