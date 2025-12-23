# GPIO Module - DE10-Lite FPGA

## Overview

The GPIO module provides bidirectional General Purpose Input/Output pins accessible via the Z-Core processor. On the DE10-Lite board, the GPIO is mapped to the onboard LEDs and switches.

> **Note**: For detailed GPIO module implementation and RTL design, see the [Z-Core repository](https://github.com/paudiaz99/Z-Core).

---

## DE10-Lite Hardware Mapping

| GPIO Pins | DE10-Lite Component | Direction |
|-----------|---------------------|-----------|
| `GPIO[9:0]` | `LEDR[9:0]` (Red LEDs) | Output |
| `GPIO[19:10]` | `SW[9:0]` (Switches) | Input |

### LED Control

The 10 red LEDs on the DE10-Lite are directly controlled by writing to the GPIO data register:

```c
// Turn on LED 0
*(volatile uint32_t*)0x04001000 = 0x00000001;

// Turn on all LEDs
*(volatile uint32_t*)0x04001000 = 0x000003FF;

// Turn off all LEDs
*(volatile uint32_t*)0x04001000 = 0x00000000;
```

### Switch Reading

The 10 slide switches can be read from the GPIO data register:

```c
// Read switch states (bits 19:10)
uint32_t switches = (*(volatile uint32_t*)0x04001000 >> 10) & 0x3FF;
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
    // Configure GPIO[9:0] as outputs
    GPIO_DIR = 0x000003FF;
    
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
    GPIO_DIR = 0x000003FF;  // Bits 9:0 = output
    
    while (1) {
        // Read switches and mirror to LEDs
        uint32_t sw = (GPIO_DATA >> 10) & 0x3FF;
        GPIO_DATA = sw;
    }
}
```

---

## Pin Assignment (Quartus)

The DE10-Lite pin assignments for GPIO are configured in the Quartus project:

| Signal | FPGA Pin | Description |
|--------|----------|-------------|
| `LEDR[0]` | PIN_A8 | Red LED 0 |
| `LEDR[1]` | PIN_A9 | Red LED 1 |
| ... | ... | ... |
| `LEDR[9]` | PIN_B11 | Red LED 9 |
| `SW[0]` | PIN_C10 | Slide Switch 0 |
| `SW[1]` | PIN_C11 | Slide Switch 1 |
| ... | ... | ... |
| `SW[9]` | PIN_B14 | Slide Switch 9 |

See `Z-Core.qsf` for complete pin assignments.
