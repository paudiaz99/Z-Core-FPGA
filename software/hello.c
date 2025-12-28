/*

Copyright (c) 2025 Pau Díaz Cuesta

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

// ================================================================
// Hello World - RISC-V RV32IM Example for Z-Core
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
