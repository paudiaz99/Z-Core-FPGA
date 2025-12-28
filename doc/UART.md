# UART Module - DE10-Lite FPGA

## Overview

The UART module provides serial communication accessible via the Z-Core processor. On the DE10-Lite board, the UART is connected through the onboard USB-Blaster for host PC communication.

> **Note**: For detailed UART module implementation and RTL design, see the [Z-Core repository](https://github.com/paudiaz99/Z-Core).

---

<div align="center">
  <img src="https://github.com/user-attachments/assets/b5b30bf8-e1a6-4933-ab1f-c4492ec62d6e" alt="centered image">
  <br>
  <sup>Uart Serial Communication with host PC.</sup>
</div>

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

## UART Library (`software/libs/uart.c`)

The UART library provides reusable functions for serial communication. Include `uart.h` and link against `uart.c` in your projects.

### `uart_putc`

Transmits a single character over UART.

```c
void uart_putc(char c);
```

- **Parameters**: `c` — Character to transmit
- **Behavior**: Writes the character to `UART_TX` and blocks until the TX buffer is empty (`tx_empty` flag in status register)

---

### `uart_puts`

Transmits a null-terminated string over UART.

```c
void uart_puts(const char *s);
```

- **Parameters**: `s` — Pointer to null-terminated string
- **Behavior**: Iterates through each character and calls `uart_putc()` for transmission

---

### `uart_getc`

Receives a single character from UART.

```c
char uart_getc(void);
```

- **Returns**: The received character (lower 8 bits of `UART_RX`)
- **Note**: This is a non-blocking read; check `RX_VALID` status flag before calling if blocking behavior is needed

---

### `uart_puthex`

Prints an unsigned 32-bit integer in hexadecimal format with `0x` prefix.

```c
void uart_puthex(unsigned int val);
```

- **Parameters**: `val` — 32-bit unsigned integer to print
- **Output**: Prints `0x` followed by 8 uppercase hex digits (e.g., `0x0000ABCD`)

---

### `uart_putint`

Prints a signed integer in decimal format.

```c
void uart_putint(int val);
```

- **Parameters**: `val` — Signed 32-bit integer to print
- **Behavior**: Handles negative numbers by printing a leading `-` sign; prints `0` for zero values

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
