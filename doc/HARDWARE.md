# Hardware and Quartus

Target: Terasic DE10-Lite, MAX 10 `10M50DAF484C7G`, CPU clock 50 MHz,
64 MiB SDRAM. The current project uses Quartus Prime Lite 25.1std.

## Wiring

| Signal | FPGA pin | Purpose |
|---|---|---|
| UART RX | W6 | Adapter TX -> board, 3.3 V |
| UART TX | V5 | Board -> adapter RX |
| host_rstn | Y11 | Optional active-low host reset; may remain unconnected |
| KEY[0] | B8 | Active-low board reset |
| MAX10_CLK1_50 | P11 | 50 MHz oscillator |

Connect a common ground. The QSF contains all SDRAM, VGA, GPIO and LED pin
assignments. UART RX and host_rstn have weak pull-ups. BREAK on the existing
UART RX wire resets the CPU after a sustained low interval; no extra reset
wire is needed. UART uses 115200 8N1 and has no RX FIFO. Console input is
paced at 30 ms per character; Linux polls RX every 5 ms.

## Memory map

| Address range | Device |
|---|---|
| `0x00000000–0x00003fff` | 16 KiB BRAM bootloader |
| `0x04000000–0x04000fff` | UART |
| `0x04001000–0x04001fff` | GPIO |
| `0x04002000–0x04002fff` | Timer |
| `0x04003000–0x04003fff` | VGA registers |
| `0x10000000–0x13ffffff` | 64 MiB SDRAM |

Linux Image loads at `0x10000000`, its entry shim at `0x13fe0000`, and its DTB
at `0x13ff0000`. The top 128 KiB is reserved. The ROM verifies checksums for
all three upload segments, drains writes, synchronizes instruction fetch and
jumps to the shim. The shim sets hart ID 0 and the DTB pointer, disables the
board timer/interrupts and enters Linux. No OpenSBI or U-Boot is involved.

VGA accepts 320×200 RGB332 pixels through packed four-pixel writes; scanout is
640×480. Linux exposes `/dev/zcore-vga`. The timer supplies the clocksource,
clockevent and UART polling tick. SDRAM uses its included PLL/controller,
FIFO IP and AXI bridge; CPU and SDRAM clock domains are connected by the CDC
bridge.

## Rebuild the FPGA

```sh
make bootloader
make fpga
./scripts/reboot.py --reprogram --bitstream output_files/Z-Core.sof
```

A bare-metal RV32 toolchain on PATH is needed only for `make bootloader`.
The checked-in MIF lanes are sufficient for compiling the project directly.
`Z-Core.qsf` and every QIP use local relative paths. CPU/peripheral RTL is in
`rtl/`, SDRAM control/FIFOs in `rtl/Sdram_Control/`, and ROM lanes in
`software/bootloader/`. The QPF includes the `Z-Core` revision, so it can also
be opened directly in Quartus.

Compilation creates `db/`, `incremental_db/` and `output_files/`; none is source.
A local compile writes `output_files/Z-Core.sof` (gitignored).

## Peripheral registers

Offsets below are relative to each device's base address. Registers use
32-bit AXI-Lite accesses.

| Device | Offset | Register |
|---|---|---|
| UART | `0x00` | TX byte, low eight bits |
| UART | `0x04` | RX byte; reading clears ready |
| UART | `0x08` | Status: bit 0 TX empty, 1 TX busy, 2 RX ready, 3 RX error |
| UART | `0x0c` | Control: bit 0 TX enable, 1 RX enable |
| UART | `0x10` | Baud divisor, low 16 bits; 27 at 50 MHz/115200 baud |
| GPIO | `0x00`, `0x04` | Data low/high; writes drive outputs, reads sample pins |
| GPIO | `0x08`, `0x0c` | Direction low/high; 1 output, 0 input |
| Timer | `0x00`, `0x04` | 64-bit timer low/high |
| Timer | `0x08` | Control: bit 0 enable, 1 count up, 2 external event, 3 compare IRQ enable |
| Timer | `0x0c`, `0x10` | 64-bit compare low/high; IRQ when timer >= compare |
| VGA | `0x00` | Framebuffer pixel address, low 16 bits |
| VGA | `0x04` | RGB332 pixel, auto-increment by one |
| VGA | `0x08` | Status bit 0: vertical blank |
| VGA | `0x0c` | Four RGB332 pixels, low byte first, auto-increment by four |

Packed VGA writes require a four-pixel-aligned framebuffer address. Read the
64-bit timer high/low/high and retry when the high values differ. Linux owns
timer and UART configuration while it runs; use its drivers for access.
