# UART Module - DE10-Lite FPGA

## Overview

The UART module provides serial communication accessible via the Z-Core processor. On the DE10-Lite board, the UART is connected through the onboard USB-Blaster for host PC communication.

<div align="center">
  <img src="https://github.com/user-attachments/assets/b5b30bf8-e1a6-4933-ab1f-c4492ec62d6e" alt="centered image">
  <br>
  <sup>Uart Serial Communication with PC.</sup>
</div>

> **Note**: For detailed UART module implementation and RTL design, see the [Z-Core repository](https://github.com/paudiaz99/Z-Core).

---

## DE10-Lite Hardware Connection

| Signal | FPGA Pin | Connection |
|--------|----------|------------|
| `uart_tx` | GPIO or USB-UART | Transmit to PC |
| `uart_rx` | GPIO or USB-UART | Receive from PC |

### Default Configuration
- **Baud Rate**: 9600
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Clock**: 50 MHz (MAX10_CLK1_50)

---

## Register Map

| Offset | Name | Description | Access |
|--------|------|-------------|--------|
| `0x00` | TX_DATA | Write byte to transmit | W |
| `0x04` | RX_DATA | Read received byte | R |
| `0x08` | STATUS | Status register | R |
| `0x0C` | CTRL | Control register | R/W |
| `0x10` | BAUD_DIV | Baud rate divisor | R/W |

### STATUS Register (0x08)

| Bit | Name | Description |
|-----|------|-------------|
| 0 | TX_EMPTY | TX buffer empty, ready to send |
| 1 | TX_BUSY | TX shift register active |
| 2 | RX_VALID | RX data available |
| 3 | RX_ERR | RX framing error |

### CTRL Register (0x0C)

| Bit | Name | Description |
|-----|------|-------------|
| 0 | TX_EN | Enable transmitter |
| 1 | RX_EN | Enable receiver |

### BAUD_DIV Register (0x10)

16-bit baud rate divisor:
```
BAUD_DIV = clock_freq / (16 * baud_rate)

For 50 MHz clock and 9600 baud:
BAUD_DIV = 50000000 / (16 * 9600) ≈ 27
```

---

## Memory Map

| Peripheral | Base Address | Size |
|------------|--------------|------|
| Memory | `0x00000000` | 4 KB |
| **UART** | `0x04000000` | 4 KB |
| GPIO | `0x04001000` | 4 KB |

---

## Usage Examples

### Transmit a Character

```c
#include <stdint.h>

#define UART_TX     (*(volatile uint32_t*)0x04000000)
#define UART_STATUS (*(volatile uint32_t*)0x04000008)

void uart_putc(char c) {
    // Wait for TX buffer empty
    while (!(UART_STATUS & 0x1));
    
    // Transmit character
    UART_TX = c;
}
```

### Transmit a String

```c
void uart_puts(const char* str) {
    while (*str) {
        uart_putc(*str++);
    }
}

void main() {
    uart_puts("Hello from Z-Core on DE10-Lite!\r\n");
}
```

### Receive a Character

```c
#define UART_RX (*(volatile uint32_t*)0x04000004)

char uart_getc() {
    // Wait for RX data available
    while (!(UART_STATUS & 0x4));
    
    return UART_RX & 0xFF;
}
```

### Echo Program

```c
void main() {
    uart_puts("UART Echo - Type characters:\r\n");
    
    while (1) {
        char c = uart_getc();
        uart_putc(c);  // Echo back
    }
}
```

---

## PC Terminal Setup

To communicate with the Z-Core via UART:

1. **Connect DE10-Lite** via USB to your PC
2. **Open a terminal program** (PuTTY, Tera Term, minicom, etc.)
3. **Configure serial settings**:
   - Port: COMx (Windows) or /dev/ttyUSBx (Linux)
   - Baud: 9600
   - Data bits: 8
   - Parity: None
   - Stop bits: 1
   - Flow control: None

### Windows (PuTTY)
1. Select "Serial" connection type
2. Enter COM port (check Device Manager)
3. Set speed to 9600
4. Open connection

### Linux (minicom)
```bash
minicom -D /dev/ttyUSB0 -b 9600
```
