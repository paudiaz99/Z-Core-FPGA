// ================================================================
// Simple GPIO Test - Minimal code to verify GPIO writes
// ================================================================

#define GPIO_BASE 0x04001000
#define GPIO_DATA (*((volatile unsigned int *)(GPIO_BASE + 0x00)))
#define GPIO_DIR (*((volatile unsigned int *)(GPIO_BASE + 0x08)))

void delay(unsigned int count) {
  for (volatile unsigned int i = 0; i < count; i++) {
    asm volatile("nop");
  }
}

int main(void) {
  unsigned int counter = 0;

  // Configure GPIO: all pins as outputs
  GPIO_DIR = 0xFF;

  // Simple loop - just toggle GPIOs
  while (1) {
    GPIO_DATA = counter & 0xFF;

    // Short delay (1000 iterations = ~3000 cycles)
    delay(1000);

    counter++;
  }

  return 0;
}
