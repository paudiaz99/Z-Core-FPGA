// ================================================================
// Hello World - RISC-V RV32I Example for Z-Core
// ================================================================

#define UART_BASE 0x04000000
#define UART_TX (*((volatile unsigned int *)(UART_BASE + 0x00)))
#define UART_RX (*((volatile unsigned int *)(UART_BASE + 0x04)))
#define UART_STAT (*((volatile unsigned int *)(UART_BASE + 0x08)))

#define GPIO_BASE 0x04001000
#define GPIO_OUT (*((volatile unsigned int *)(GPIO_BASE + 0x00)))
#define GPIO_IN (*((volatile unsigned int *)(GPIO_BASE + 0x04)))
#define GPIO_DIR (*((volatile unsigned int *)(GPIO_BASE + 0x08)))

// Simple delay function
void delay(unsigned int count) {
  for (unsigned int i = 0; i < count; i++) {
    asm volatile("nop");
  }
}

// UART functions
void uart_putc(char c) {
  UART_TX = (unsigned int)c;
  // Wait for transmission to complete (tx_empty = bit 0)
  while (!(UART_STAT & 0x01))
    ; // Wait until tx_empty is 1
}

void uart_puts(const char *s) {
  while (*s) {
    uart_putc(*s++);
  }
}

char uart_getc(void) { return (char)(UART_RX & 0xFF); }

void uart_puthex(unsigned int val) {
  const char hex[] = "0123456789ABCDEF";
  uart_puts("0x");
  for (int i = 28; i >= 0; i -= 4) {
    uart_putc(hex[(val >> i) & 0xF]);
  }
}

// Main program
int main(void) {
  // Configure GPIO: all pins as outputs
  GPIO_DIR = 0xFF;
  GPIO_OUT = 0x00;

  // Print startup message
  uart_puts("\r\n");
  uart_puts("========================================\r\n");
  uart_puts("  Z-Core RISC-V Processor\r\n");
  uart_puts("  RV32I @ 50 MHz\r\n");
  uart_puts("  DE10-Lite FPGA\r\n");
  uart_puts("========================================\r\n");
  uart_puts("\r\n");

  unsigned int counter = 0;

  while (1) {
    // Print counter
    uart_puts("Counter: ");
    uart_puthex(counter);
    uart_puts("\r\n");

    // Blink LEDs via GPIO
    GPIO_OUT = counter & 0xFF;

    // Delay
    delay(500000); // reduced delay since loop above takes time

    counter++;
  }

  return 0;
}
