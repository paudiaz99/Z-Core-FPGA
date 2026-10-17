	.equ UART_BASE, 0x04000000
	.equ UART_TX,   0x00
	.equ UART_STAT, 0x08

	.section .text.start
	.global _start
_start:

	li   t0, UART_BASE

1:	lw   t1, UART_STAT(t0)
	andi t1, t1, 1
	beqz t1, 1b
	li   t1, 'Z'
	sw   t1, UART_TX(t0)

2:	lw   t1, UART_STAT(t0)
	andi t1, t1, 1
	beqz t1, 2b
	li   t1, '>'
	sw   t1, UART_TX(t0)

3:	lw   t1, UART_STAT(t0)
	andi t1, t1, 1
	beqz t1, 3b
	li   t1, '\r'
	sw   t1, UART_TX(t0)
4:	lw   t1, UART_STAT(t0)
	andi t1, t1, 1
	beqz t1, 4b
	li   t1, '\n'
	sw   t1, UART_TX(t0)


	la sp, __stacktop

	la   t0, _trap_handler
	csrw mtvec, t0

	la a0, _sbss
	la a1, _ebss
	bge a0, a1, end_init_bss
loop_init_bss:
	sw zero, 0(a0)
	addi a0, a0, 4
	blt a0, a1, loop_init_bss
end_init_bss:

	call main

.global	_exit
_exit:
	j _exit



	.section .text
	.global _trap_handler
	.balign 4
_trap_handler:
	mv   a2, ra

	li   a0, UART_BASE

	csrr a3, mcause
	csrr a4, mepc
	csrr a5, mtval

	mv   t1, a2

_th_nl1:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_nl1
	li   t0, '\r'
	sw   t0, UART_TX(a0)
_th_nl1b:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_nl1b
	li   t0, '\n'
	sw   t0, UART_TX(a0)

_th_T:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_T
	li   t0, 'T'
	sw   t0, UART_TX(a0)

_th_R:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_R
	li   t0, 'R'
	sw   t0, UART_TX(a0)

_th_A:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_A
	li   t0, 'A'
	sw   t0, UART_TX(a0)

_th_P:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_P
	li   t0, 'P'
	sw   t0, UART_TX(a0)

_th_sp1:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_sp1
	li   t0, ' '
	sw   t0, UART_TX(a0)

_th_C:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_C
	li   t0, 'C'
	sw   t0, UART_TX(a0)

_th_eq1:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_eq1
	li   t0, '='
	sw   t0, UART_TX(a0)

	li   a1, 8
_th_mc_loop:
	addi a2, a1, -1
	slli a2, a2, 2
	srl  a2, a3, a2
	andi a2, a2, 0xF
	addi a2, a2, '0'
	li   t0, '9'
	ble  a2, t0, _th_mc_digit
	addi a2, a2, 7
_th_mc_digit:
_th_mc_tx:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_mc_tx
	sw   a2, UART_TX(a0)
	addi a1, a1, -1
	bnez a1, _th_mc_loop

_th_sp2:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_sp2
	li   t0, ' '
	sw   t0, UART_TX(a0)
_th_E:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_E
	li   t0, 'E'
	sw   t0, UART_TX(a0)
_th_eq2:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_eq2
	li   t0, '='
	sw   t0, UART_TX(a0)

	li   a1, 8
_th_ep_loop:
	addi a2, a1, -1
	slli a2, a2, 2
	srl  a2, a4, a2
	andi a2, a2, 0xF
	addi a2, a2, '0'
	li   t0, '9'
	ble  a2, t0, _th_ep_digit
	addi a2, a2, 7
_th_ep_digit:
_th_ep_tx:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_ep_tx
	sw   a2, UART_TX(a0)
	addi a1, a1, -1
	bnez a1, _th_ep_loop

_th_sp3:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_sp3
	li   t0, ' '
	sw   t0, UART_TX(a0)
_th_V:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_V
	li   t0, 'V'
	sw   t0, UART_TX(a0)
_th_eq3:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_eq3
	li   t0, '='
	sw   t0, UART_TX(a0)

	li   a1, 8
_th_mt_loop:
	addi a2, a1, -1
	slli a2, a2, 2
	srl  a2, a5, a2
	andi a2, a2, 0xF
	addi a2, a2, '0'
	li   t0, '9'
	ble  a2, t0, _th_mt_digit
	addi a2, a2, 7
_th_mt_digit:
_th_mt_tx:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_mt_tx
	sw   a2, UART_TX(a0)
	addi a1, a1, -1
	bnez a1, _th_mt_loop

_th_sp4:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_sp4
	li   t0, ' '
	sw   t0, UART_TX(a0)
_th_Ra:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_Ra
	li   t0, 'R'
	sw   t0, UART_TX(a0)
_th_eq4:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_eq4
	li   t0, '='
	sw   t0, UART_TX(a0)

	li   a1, 8
_th_ra_loop:
	addi a2, a1, -1
	slli a2, a2, 2
	srl  a2, t1, a2
	andi a2, a2, 0xF
	addi a2, a2, '0'
	li   t0, '9'
	ble  a2, t0, _th_ra_digit
	addi a2, a2, 7
_th_ra_digit:
_th_ra_tx:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_ra_tx
	sw   a2, UART_TX(a0)
	addi a1, a1, -1
	bnez a1, _th_ra_loop

_th_nl2:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_nl2
	li   t0, '\r'
	sw   t0, UART_TX(a0)
_th_nl2b:
	lw   t0, UART_STAT(a0)
	andi t0, t0, 1
	beqz t0, _th_nl2b
	li   t0, '\n'
	sw   t0, UART_TX(a0)

_trap_hang:
	j    _trap_hang
