// ================================================================
// GPIO Test - RISC-V RV32I Example for Z-Core
// ================================================================

#define GPIO_BASE 0x04001000
#define GPIO_OUT  (*((volatile unsigned int*)(GPIO_BASE + 0x00)))
#define GPIO_IN   (*((volatile unsigned int*)(GPIO_BASE + 0x04)))
#define GPIO_DIR  (*((volatile unsigned int*)(GPIO_BASE + 0x08)))

// Simple delay function
void delay(unsigned int count) {
    for(unsigned int i = 0; i < count; i++) {
        asm volatile ("nop");
    }
}

// Main program
int main(void) {
    // Configure GPIO: all pins as outputs
    GPIO_DIR = 0xFF;
    GPIO_OUT = 0x00;
    
    while(1) {
        // Toggle all LEDs
        GPIO_OUT = ~GPIO_OUT;
        
        // Delay
        delay(1000000);
    }
    
    return 0;
}
