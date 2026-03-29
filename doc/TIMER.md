# 64-bit Timer/Counter

The Z-Core Timer is a versatile 64-bit peripheral that can function as a real-time clock (RTC), a general-purpose timer, or an external event counter. It is integrated with the CPU and can trigger Machine Timer Interrupts (`mtip`).

## Features

- **64-bit Counter**: Implemented as two 32-bit registers (Low/High).
- **Comparator**: Triggers an interrupt when the counter reaches a specific value.
- **Multimode**: Supports counting system clock cycles (Timer mode) or external signal edges (Counter mode).
- **Direction**: Supports counting up or down.

## Register Map

Base Address: `0x04002000`

| Offset | Name | Type | Description |
|--------|------|------|-------------|
| `0x00` | `TIMER_LO` | R/W | Counter bits [31:0]. Writing loads the value immediately. |
| `0x04` | `TIMER_HI` | R/W | Counter bits [63:32]. Writing loads the value immediately. |
| `0x08` | `TIMER_CTRL`| R/W | Control register. See bit definitions below. |
| `0x0C` | `TIMECMP_LO`| R/W | Compare value bits [31:0]. |
| `0x10` | `TIMECMP_HI`| R/W | Compare value bits [63:32]. |

## Control Register (`TIMER_CTRL`)

| Bit | Name | Description |
|-----|------|-------------|
| `0` | `ENABLE` | 1: Enable counter, 0: Disable. |
| `1` | `DIR`    | 1: Count Up, 0: Count Down. |
| `2` | `MODE`   | 0: Timer mode (cycles), 1: Counter mode (external edges). |
| `3` | `IE`     | Interrupt Enable. If 1, triggers CPU `mtip` when `TIMER >= TIMECMP`. |

## Usage Note

The timer is connected to the `mtip` (Machine Timer Interrupt) input of the Z-Core CSR file. To use interrupts:
1. Set the desired compare value in `TIMECMP_LO/HI`.
2. Enable the interrupt in `TIMER_CTRL` (Bit 3).
3. Enable interrupts in the CPU `mstatus` and `mie` CSRs.
