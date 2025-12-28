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
// Multiplication/Division Test - Z-Core RV32IM
// Uses ONLY uart_puthex (bit shifts) to avoid using DIV for output
// ================================================================

#include "libs/uart.h"

#define GPIO_OUT (*((volatile unsigned int *)0x04001000))
#define GPIO_DIR (*((volatile unsigned int *)0x04001008))

void delay(int n) {
  for (int i = 0; i < n; i++) asm volatile("nop");
}

int p = 0, f = 0;

// Use noinline to prevent constant folding
void __attribute__((noinline)) tm(int a, int b, int exp) {
  volatile int va = a;
  volatile int vb = b;
  int r = va * vb;
  
  // Output in hex to avoid using DIV/REM for printing
  uart_puthex((unsigned int)a); uart_putc('*'); 
  uart_puthex((unsigned int)b); uart_putc('='); 
  uart_puthex((unsigned int)r);
  uart_puts(" exp:"); uart_puthex((unsigned int)exp);
  
  if (r == exp) { uart_puts(" OK\r\n"); p++; } 
  else { uart_puts(" FAIL\r\n"); f++; }
}

void __attribute__((noinline)) td(int a, int b, int exp) {
  volatile int va = a;
  volatile int vb = b;
  int r = va / vb;
  
  uart_puthex((unsigned int)a); uart_putc('/'); 
  uart_puthex((unsigned int)b); uart_putc('='); 
  uart_puthex((unsigned int)r);
  uart_puts(" exp:"); uart_puthex((unsigned int)exp);
  
  if (r == exp) { uart_puts(" OK\r\n"); p++; } 
  else { uart_puts(" FAIL\r\n"); f++; }
}

void __attribute__((noinline)) tr(int a, int b, int exp) {
  volatile int va = a;
  volatile int vb = b;
  int r = va % vb;
  
  uart_puthex((unsigned int)a); uart_putc('%'); 
  uart_puthex((unsigned int)b); uart_putc('='); 
  uart_puthex((unsigned int)r);
  uart_puts(" exp:"); uart_puthex((unsigned int)exp);
  
  if (r == exp) { uart_puts(" OK\r\n"); p++; } 
  else { uart_puts(" FAIL\r\n"); f++; }
}

int main(void) {
  GPIO_DIR = 0xFF;
  GPIO_OUT = 0x01;

  uart_puts("\r\n=== Z-Core MUL/DIV Test ===\r\n");
  uart_puts("(All values in hex to avoid DIV in output)\r\n\r\n");

  uart_puts("-- MUL --\r\n");
  tm(5, 7, 35);           // 0x5 * 0x7 = 0x23
  tm(12, 12, 144);        // 0xC * 0xC = 0x90
  tm(100, 100, 10000);    // 0x64 * 0x64 = 0x2710
  tm(0, 123, 0);          // 0x0 * 0x7B = 0x0
  tm(256, 256, 65536);    // 0x100 * 0x100 = 0x10000

  uart_puts("\r\n-- DIV --\r\n");
  td(35, 7, 5);           // 0x23 / 0x7 = 0x5
  td(100, 10, 10);        // 0x64 / 0xA = 0xA
  td(1000, 7, 142);       // 0x3E8 / 0x7 = 0x8E
  td(0, 5, 0);            // 0x0 / 0x5 = 0x0
  td(65536, 256, 256);    // 0x10000 / 0x100 = 0x100

  uart_puts("\r\n-- REM --\r\n");
  tr(35, 7, 0);           // 0x23 % 0x7 = 0x0
  tr(36, 7, 1);           // 0x24 % 0x7 = 0x1
  tr(1000, 7, 6);         // 0x3E8 % 0x7 = 0x6
  tr(100, 3, 1);          // 0x64 % 0x3 = 0x1

  uart_puts("\r\n-- UNSIGNED --\r\n");
  volatile unsigned int ua = 4000000000U;  // 0xEE6B2800
  volatile unsigned int ub = 1000000U;     // 0x000F4240
  
  unsigned int ru = ua / ub;
  uart_puts("DIVU="); uart_puthex(ru);
  uart_puts(" exp:"); uart_puthex(4000U);
  if (ru == 4000U) { uart_puts(" OK\r\n"); p++; } else { uart_puts(" FAIL\r\n"); f++; }
  
  ru = ua % ub;
  uart_puts("REMU="); uart_puthex(ru);
  uart_puts(" exp:"); uart_puthex(0U);
  if (ru == 0U) { uart_puts(" OK\r\n"); p++; } else { uart_puts(" FAIL\r\n"); f++; }

  uart_puts("\r\n-- COMBINED --\r\n");
  volatile int x = 10;
  int poly = 3 * x * x + 2 * x + 1;  // 3*100 + 20 + 1 = 321 = 0x141
  uart_puts("3x^2+2x+1="); uart_puthex((unsigned int)poly);
  uart_puts(" exp:"); uart_puthex(321U);
  if (poly == 321) { uart_puts(" OK\r\n"); p++; } else { uart_puts(" FAIL\r\n"); f++; }

  volatile int a = 12345;  // 0x3039
  volatile int b = 67;     // 0x43
  int q = a / b;           // 184 = 0xB8
  int r = a % b;           // 17 = 0x11
  int identity = q * b + r;
  uart_puts("(a/b)*b+a%b="); uart_puthex((unsigned int)identity);
  uart_puts(" exp:"); uart_puthex((unsigned int)a);
  if (identity == a) { uart_puts(" OK\r\n"); p++; } else { uart_puts(" FAIL\r\n"); f++; }

  uart_puts("\r\n=================\r\n");
  uart_puts("PASS:"); uart_puthex((unsigned int)p); 
  uart_puts(" FAIL:"); uart_puthex((unsigned int)f); 
  uart_puts("\r\n");

  GPIO_OUT = (f == 0) ? 0xAA : 0x55;
  uart_puts(f == 0 ? "ALL PASSED\r\n" : "SOME FAILED\r\n");

  while (1);
  return 0;
}
