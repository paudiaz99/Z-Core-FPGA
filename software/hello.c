// ================================================================
// Hello World - RISC-V RV32I Example for Z-Core
// ================================================================

#include "libs/uart.h"

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

// Main program
int main(void) {
  // Configure GPIO: all pins as outputs
  GPIO_DIR = 0xFF; // Set direction to output
  GPIO_OUT = 0x00; // Initialize LEDs to off

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
