#include <stdint.h>

#include "config.h"
#include "mini-printf.h"


void
console_init(void)
{
	UART_BAUD_DIV = 27;
}

void __attribute__((noinline))
console_putchar(char c)
{
	if (c == '\n') {
		while (!(UART_STAT & UART_STAT_TX_EMPTY))
			;
		UART_TX = '\r';
		while (!(UART_STAT & UART_STAT_TX_EMPTY))
			;
	}
	while (!(UART_STAT & UART_STAT_TX_EMPTY))
		;
	UART_TX = (uint32_t)c;
	while (!(UART_STAT & UART_STAT_TX_EMPTY))
		;
}

char
console_getchar(void)
{
	while (!(UART_STAT & UART_STAT_RX_VALID))
		;
	return (char)(UART_RX & 0xFF);
}

int
console_getchar_nowait(void)
{
	if (!(UART_STAT & UART_STAT_RX_VALID))
		return -1;
	return UART_RX & 0xFF;
}

void
console_puts(const char *p)
{
	char c;
	while ((c = *(p++)) != 0x00)
		console_putchar(c);
}

int
console_printf(const char *fmt, ...)
{
	static char _printf_buf[128];
        va_list va;
        int l;

        va_start(va, fmt);
        l = mini_vsnprintf(_printf_buf, 128, fmt, va);
        va_end(va);

	console_puts(_printf_buf);

	return l;
}
