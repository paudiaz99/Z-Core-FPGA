# GPIO Module - DE10-Lite FPGA

## Overview

The GPIO module provides bidirectional General Purpose Input/Output pins accessible via the Z-Core processor. On the DE10-Lite board, the GPIO is mapped to the onboard LEDs and switches.

> **Note**: For detailed GPIO module implementation and RTL design, see the [Z-Core repository](https://github.com/paudiaz99/Z-Core).

---

## DE10-Lite Hardware Mapping

| GPIO Pins | DE10-Lite Component | Direction |
|-----------|---------------------|-----------|
| `GPIO[7:0]` | `GPIO[7:0]` | Output |

You can also configure the GPIOs to control the LEDs and switches on the DE10-Lite board.

### [7:0] LED Control

The LEDs on the DE10-Lite are directly controlled by writing to the GPIO data register:

```c
// Turn on LED 0
*(volatile uint32_t*)0x04001000 = 0x00000001;

// Turn on all LEDs
*(volatile uint32_t*)0x04001000 = 0x000000FF;

// Turn off all LEDs
*(volatile uint32_t*)0x04001000 = 0x00000000;
```

### [15:8] Switch Reading

The switches on the DE10-Lite can be read from the GPIO data register:

```c
// Read switch states (bits 15:8)
uint32_t switches = (*(volatile uint32_t*)0x04001000 >> 8) & 0xFF;
```

---

## Register Map

| Offset | Name | Description | Access |
|--------|------|-------------|--------|
| `0x00` | DATA_LOW | GPIO[31:0] data | R/W |
| `0x04` | DATA_HIGH | GPIO[63:32] data | R/W |
| `0x08` | DIR_LOW | GPIO[31:0] direction | R/W |
| `0x0C` | DIR_HIGH | GPIO[63:32] direction | R/W |

### DATA Registers (0x00, 0x04)
- **Write**: Sets the output value for pins configured as outputs
- **Read**: Returns current GPIO pin states

### DIR Registers (0x08, 0x0C)
- **Bit = 1**: Pin configured as **output**
- **Bit = 0**: Pin configured as **input** (high-impedance)

---

## Memory Map

| Peripheral | Base Address | Size |
|------------|--------------|------|
| Memory | `0x00000000` | 4 KB |
| UART | `0x04000000` | 4 KB |
| **GPIO** | `0x04001000` | 4 KB |

---

## Usage Examples

### Blink LED Pattern

```c
#include <stdint.h>

#define GPIO_DATA (*(volatile uint32_t*)0x04001000)
#define GPIO_DIR  (*(volatile uint32_t*)0x04001008)

void main() {
    // Configure GPIO[7:0] as outputs
    GPIO_DIR = 0x000000FF;
    
    while (1) {
        GPIO_DATA = 0x00000055;  // Alternating pattern
        delay(500000);
        GPIO_DATA = 0x000000AA;  // Inverted pattern
        delay(500000);
    }
}
```

### Read Switches, Display on LEDs

```c
void main() {
    // Configure LEDs as output, switches as input
    GPIO_DIR = 0x000000FF;  // Bits 7:0 = output, 15:8 = input
    
    while (1) {
        // Read switches (15:8) and mirror to LEDs (7:0)
        uint32_t sw = (GPIO_DATA >> 8) & 0xFF; 
        GPIO_DATA = sw;
    }
}
```
