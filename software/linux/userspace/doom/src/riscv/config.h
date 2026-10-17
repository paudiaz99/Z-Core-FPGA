#pragma once

#include <stdint.h>

#define UART_BASE       0x04000000
#define GPIO_BASE       0x04001000
#define TIMER_BASE      0x04002000
#define VGA_BASE        0x04003000

#define UART_TX         (*(volatile uint32_t *)(UART_BASE + 0x00))
#define UART_RX         (*(volatile uint32_t *)(UART_BASE + 0x04))
#define UART_STAT       (*(volatile uint32_t *)(UART_BASE + 0x08))
#define UART_CTRL       (*(volatile uint32_t *)(UART_BASE + 0x0C))
#define UART_BAUD_DIV   (*(volatile uint32_t *)(UART_BASE + 0x10))

#define UART_STAT_TX_EMPTY  0x01
#define UART_STAT_TX_BUSY   0x02
#define UART_STAT_RX_VALID  0x04

#define TIMER_LO        (*(volatile uint32_t *)(TIMER_BASE + 0x00))
#define TIMER_HI        (*(volatile uint32_t *)(TIMER_BASE + 0x04))
#define TIMER_CTRL_REG  (*(volatile uint32_t *)(TIMER_BASE + 0x08))

#define VGA_FB_ADDR     (*(volatile uint32_t *)(VGA_BASE + 0x00))
#define VGA_FB_DATA     (*(volatile uint32_t *)(VGA_BASE + 0x04))
#define VGA_FB_STATUS   (*(volatile uint32_t *)(VGA_BASE + 0x08))

#define VGA_WIDTH       320
#define VGA_HEIGHT      200

#define WAD_BASE        0x12010000
#define WAD_SIZE        4196020

#define SDRAM_BASE      0x10000000
