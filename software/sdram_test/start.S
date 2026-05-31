/*
 * SDRAM read diagnostic — pure assembly, NO .rodata loads.
 *
 * All output formatting uses immediate values in registers.
 * This eliminates SDRAM data-load corruption from the diagnostic path.
 *
 * Tests:
 *   1. Print "S>" (proves instruction fetch from SDRAM works)
 *   2. Read own first 8 words via LW, print as hex
 *   3. Write 0xDEADBEEF to SDRAM scratch, read back, print
 *   4. Write 0x12345678, read back individual bytes via LBU, print
 *   5. Read code bytes via LBU (test byte lanes from .text)
 */

    .equ UART_BASE, 0x04000000
    .equ UART_TX,   0x00
    .equ UART_STAT, 0x08

    .equ SDRAM_BASE,   0x10000000
    .equ SCRATCH_ADDR, 0x10800000  /* well past code, safe scratch */

    .section .text.start
    .global _start
_start:

    /* ============================================================
     * Subroutines at the top (jumped over by _start)
     * Using a0 = char, t0 = UART_BASE (preserved by callers)
     * ============================================================ */
    j _real_start

    /* ---- putc: send a0[7:0] to UART. Clobbers t1. ---- */
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

    /* ---- puthex32: print s1 as 8 hex digits. Clobbers a0,t1,t2,t3. ---- */
puthex32:
    addi sp, sp, -4
    sw   ra, 0(sp)
    li   t2, 28            /* shift amount, counts down */
.Lhex_loop:
    srl  t3, s1, t2        /* shift right by t2 */
    andi t3, t3, 0xF       /* mask nibble */
    li   a0, '0'
    add  a0, a0, t3        /* '0' + nibble */
    li   t1, '9'
    ble  a0, t1, .Lhex_ok  /* if <= '9', fine */
    addi a0, a0, 7         /* skip ':' to 'A': '9'+1+7 = 'A' */
.Lhex_ok:
    call putc
    addi t2, t2, -4
    bge  t2, zero, .Lhex_loop
    lw   ra, 0(sp)
    addi sp, sp, 4
    ret

    /* ---- print_crlf: send \r\n ---- */
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

    /* ============================================================ */
_real_start:
    /* Set stack pointer */
    li   sp, 0x10900000

    /* ---- Banner: "S>\n" ---- */
    li   a0, 'S'
    call putc
    li   a0, '>'
    call putc
    call print_crlf

    /* ============================================================
     * TEST 1: LW from own code (first 8 words at SDRAM_BASE)
     *
     * Print "LW:" then 8 hex words separated by spaces.
     * Compare with objdump of this binary to verify.
     * ============================================================ */
    li   a0, 'L'
    call putc
    li   a0, 'W'
    call putc
    li   a0, ':'
    call putc

    li   s0, SDRAM_BASE    /* address to read */
    li   s2, 8             /* 8 words */
.Llw_loop:
    li   a0, ' '
    call putc
    lw   s1, 0(s0)         /* LW from SDRAM */
    call puthex32           /* print hex (uses s1) */
    addi s0, s0, 4
    addi s2, s2, -1
    bnez s2, .Llw_loop
    call print_crlf

    /* ============================================================
     * TEST 2: Write 0xDEADBEEF to scratch, read back
     *
     * Print "WR: DEADBEEF RB: <readback>"
     * ============================================================ */
    li   a0, 'W'
    call putc
    li   a0, 'R'
    call putc
    li   a0, ':'
    call putc
    li   a0, ' '
    call putc

    li   s0, SCRATCH_ADDR
    li   s1, 0xDEADBEEF    /* value to write (uses LUI+ADDI) */
    /* Actually build 0xDEADBEEF: lui + addi */
    lui  s1, 0xDEADB       /* s1 = 0xDEADB000 */
    addi s1, s1, -273      /* 0xEEF = 4096 - 273... let me use ori */
    /* Simpler: build in two halves */
    lui  s1, 0xDEADE       /* s1 = 0xDEADE000 */
    addi s1, s1, -0x111    /* s1 = 0xDEADE000 - 0x111 = 0xDEADCEEF? No. */
    /* Just use the correct encoding: */
    li   s1, 0xDEADBEEF    /* assembler handles it */
    call puthex32
    sw   s1, 0(s0)         /* write to SDRAM */

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

    lw   s1, 0(s0)         /* read back */
    call puthex32
    call print_crlf

    /* ============================================================
     * TEST 3: Write 0x44332211 to scratch, read back bytes via LBU
     *
     * Print "B0:<xx> B1:<xx> B2:<xx> B3:<xx>"
     * Expected: B0:11 B1:22 B2:33 B3:44 (little-endian)
     *
     * This is the CRITICAL test: do LBU byte lanes work?
     * ============================================================ */
    li   s0, SCRATCH_ADDR
    li   s1, 0x44332211    /* assembler handles encoding */
    sw   s1, 0(s0)

    /* Read and print each byte */
    li   s2, 0             /* byte index */
.Lbyte_loop:
    li   a0, 'B'
    call putc
    /* print byte index as digit */
    li   a0, '0'
    add  a0, a0, s2
    call putc
    li   a0, ':'
    call putc

    add  t3, s0, s2        /* byte address = scratch + index */
    lbu  s1, 0(t3)         /* load single byte from SDRAM */
    /* Print as 2 hex digits */
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

    /* ============================================================
     * TEST 4: Read first 8 bytes of own code via LBU
     *
     * Print "CB: <b0> <b1> <b2> <b3> <b4> <b5> <b6> <b7>"
     * Compare with objdump byte-by-byte.
     * ============================================================ */
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
    /* Print as 2 hex digits */
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

    /* ============================================================
     * TEST 5: Write 0xAABBCCDD, read full word, then read bytes
     *
     * Verifies LW vs LBU consistency from same address
     * ============================================================ */
    li   a0, 'F'
    call putc
    li   a0, 'W'
    call putc
    li   a0, ':'
    call putc
    li   a0, ' '
    call putc

    li   s0, SCRATCH_ADDR
    addi s0, s0, 16       /* different scratch location */
    li   s1, 0xAABBCCDD   /* assembler handles it */
    sw   s1, 0(s0)
    lw   s1, 0(s0)
    call puthex32

    li   a0, ' '
    call putc
    li   a0, 'B'
    call putc
    li   a0, ':'
    call putc

    /* Read all 4 bytes individually */
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

    /* ---- Done: print "OK" and halt ---- */
    li   a0, 'O'
    call putc
    li   a0, 'K'
    call putc
    call print_crlf

.global _exit
_exit:
    j _exit
