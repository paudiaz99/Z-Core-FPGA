# Z-Core Software Guide

How to build, upload, and run programs on the Z-Core RISC-V SoC (DE10-Lite).

---

## 1. SoC Address Map

| Peripheral | Base Address | Size | Notes |
|------------|-------------|------|-------|
| BRAM | `0x0000_0000` | 16 KB | On-chip block RAM (bootloader + small apps) |
| UART | `0x0400_0000` | 4 KB | 115200 8N1, TX/RX/STATUS/CTRL/BAUD_DIV |
| GPIO | `0x0400_1000` | 4 KB | 16 bidirectional pins |
| Timer | `0x0400_2000` | 4 KB | 64-bit free-running @ 50 MHz |
| VGA | `0x0400_3000` | 4 KB | 320x200 RGB332 framebuffer |
| SDRAM | `0x1000_0000` | 64 MB | IS42S16320D, via AXI-CDC bridge |

### BRAM Layout

```
0x0000_0000 +------------------+
            | Bootloader (4 KB)|  <- Loaded from MIF at synthesis
0x0000_1000 +------------------+
            | App space (12 KB)|  <- Uploaded by bootloader
0x0000_4000 +------------------+
            | (stack top)      |
```

### SDRAM Layout (for large programs like DOOM)

```
0x1000_0000 +------------------+
            | .text + .rodata  |  Code + read-only data
            | .data / .bss     |  Initialized and zero data
            | heap →           |  Grows upward from _heap_start
            |                  |
            |         ← stack  |  Grows down from 0x11FF_0000
0x11FE_0000 +------------------+  Stack bottom (64 KB stack)
            | (guard region)   |
0x11FF_0000 +------------------+  Stack top
            |                  |
0x1201_0000 +------------------+
            | WAD / assets     |  doom1.wad loaded by bootloader (~4.2 MB)
0x1241_0000 +------------------+
```

---

## 2. Toolchain

The RISC-V cross-compiler prefix is `riscv32-unknown-elf-`, targeting `rv32im_zicsr` with `ilp32` ABI (soft-float, no FPU).

```bash
riscv32-unknown-elf-gcc -march=rv32im_zicsr -mabi=ilp32 ...
```

---

## 3. Two Ways to Run Software

### Option A: On-Chip BRAM (small programs, up to 12 KB)

Programs are loaded to `0x0000_1000` (app space in BRAM). The bootloader receives the binary over UART and jumps to it. This is the default mode for simple test programs.

**Characteristics:**
- Maximum 12 KB binary size
- Best for: hello world, UART tests, GPIO tests, peripheral drivers

### Option B: SDRAM (large programs, up to ~9 MB code + data)

Programs are loaded to `0x1000_0000` in SDRAM via the multi-segment bootloader. The CPU fetches instructions from SDRAM through the AXI bus and CDC bridge.

**Characteristics:**
- Up to ~9 MB for code + data + heap + stack
- Additional segments (e.g. WAD files) can be loaded alongside
- Higher fetch latency
- Instruction cache (256 lines) reduces average latency
- Best for: DOOM, large applications, anything that doesn't fit in 12 KB

---

## 4. Building Programs

### Basic Program (BRAM target)

```bash
cd software/

# Build a program (produces .elf, .bin, .hex, .mif)
make hello.bin

# Build for bootloader upload (uses linker_app.ld, loads at 0x1000)
make APP=1 hello.bin
```

The `APP=1` flag uses `linker_app.ld` which places code at `0x1000` (BRAM app space).

### Bootloader

```bash
cd software/bootloader/
make all      # Produces bootloader.mif + byte-lane MIFs
```

The bootloader MIF files (`software/bootloader_byte{0-3}.mif`) are baked into the FPGA bitstream. After modifying the bootloader, you must **re-synthesize in Quartus** for changes to take effect.

### DOOM (SDRAM target)

```bash
cd software/doom_riscv/src/riscv/
make clean && make all    # Produces doom-zcore.bin (~300 KB)
```

---

## 5. Uploading Programs

The upload tool (`software/upload.py`) communicates with the bootloader over UART using a multi-segment protocol.

### Prerequisites

- Bootloader v2.0 must be synthesized into the FPGA (re-synthesize after `make all` in `software/bootloader/`)
- Serial port connected (e.g. `/dev/ttyUSB0`)
- Board powered and not in reset (KEY[0] high)

### Example: Hello World (BRAM)

```bash
cd software/

# Build for bootloader upload
make APP=1 hello.bin

# Upload to BRAM app space (0x1000) and monitor output
python3 upload.py /dev/ttyUSB0 hello.bin
```

This loads `hello.bin` to `0x1000` (default base) and jumps to it. The program runs from on-chip BRAM.

### Example: SDRAM Test

```bash
cd software/

# Build SDRAM test for BRAM execution (test code runs from BRAM, tests SDRAM)
make APP=1 sdram_test.bin

# Upload and monitor
python3 upload.py /dev/ttyUSB0 sdram_test.bin
```

Note: `sdram_test.bin` itself runs from BRAM but reads/writes SDRAM at `0x1000_0000+`.

### Example: Program Running from SDRAM

```bash
cd software/

# Build a program targeted at SDRAM (needs a linker script starting at 0x10000000)
# Then upload to SDRAM:
python3 upload.py /dev/ttyUSB0 my_program.bin --base 0x10000000
```

### Example: DOOM (Multi-Segment, SDRAM)

#### Getting doom1.wad

`doom1.wad` is a ~4.2 MB binary not included in the repository. Obtain it from one of these sources:

**Option A — FreeDOOM (open-source, always legal):**
```bash
# Ubuntu/Debian
sudo apt install freedoom
# WAD is at: /usr/share/games/doom/freedoom1.wad
```
If using FreeDOOM, update `WAD_SIZE` in `software/doom_riscv/src/riscv/config.h` to match its
actual size, and change `"doom1.wad"` to `"freedoom1.wad"` in `libc_backend.c`.

**Option B — DOOM shareware v1.9 (id Software, freely distributable):**

Search the Internet Archive for "doom shareware 1.9". Extract `doom1.wad` from the zip
(inside `doom19s/doom1.wad`).

Verify you have the correct file:
```bash
md5sum doom1.wad
# Expected: f0cefca49926d00903cf57551d901abe  (shareware v1.9)
```

**Option C — Steam / GOG (if you own DOOM):**

Find `doom.wad` in the game install directory.

#### Building and Uploading

```bash
# Build DOOM
cd software/doom_riscv/src/riscv/
make all

# Upload DOOM code + WAD to SDRAM (two segments)
cd software/
python3 upload.py /dev/ttyUSB0 \
    --segments doom_riscv/src/riscv/doom-zcore.bin@0x10000000 \
               doom_riscv/doom1.wad@0x12010000 \
    --entry 0x10000000

# With high-speed baud (460800, ~4x faster — recommended):
python3 upload.py /dev/ttyUSB0 \
    --segments doom_riscv/src/riscv/doom-zcore.bin@0x10000000 \
               doom_riscv/doom1.wad@0x12010000 \
    --entry 0x10000000 \
    --fast
```

Estimated upload times for the full ~4.5 MB payload:
- 115200 baud: ~7 minutes
- 460800 baud: ~1.5 minutes

### Upload Protocol (v2.0)

```
1. Sync:     Host sends 0x5A, device replies 0xA5 (at 115200 baud)
2. Baud:     Host sends baud_div (4 bytes LE); 0 = keep 115200
3. Segments: Host sends count (1 byte)
4. Per segment:
   - base address (4 bytes LE)
   - size (4 bytes LE)
   - payload (size bytes)
   - checksum (4 bytes LE, sum of all payload bytes)
5. Entry:    Host sends entry point (4 bytes LE), device jumps
```

### Upload Options

| Flag | Description |
|------|-------------|
| `--base ADDR` | Load address for single-file mode (default: `0x1000`) |
| `--segments FILE@ADDR ...` | Multi-segment upload |
| `--entry ADDR` | Jump address (default: base of first segment) |
| `--fast` | Switch to 460800 baud after sync |
| `--upload-baud RATE` | Custom upload baud rate |
| `-n` / `--no-terminal` | Exit after upload (don't monitor UART) |

---

## 6. Peripheral Register Reference

### UART (`0x0400_0000`)

| Offset | Register | Access | Description |
|--------|----------|--------|-------------|
| `0x00` | TX | W | Transmit data (byte) |
| `0x04` | RX | R | Receive data (byte) |
| `0x08` | STATUS | R | Bit 0: TX empty, Bit 2: RX valid |
| `0x0C` | CTRL | R/W | Control register |
| `0x10` | BAUD_DIV | R/W | Baud divisor: `50e6 / (16 * baud)` |

### Timer (`0x0400_2000`)

| Offset | Register | Access | Description |
|--------|----------|--------|-------------|
| `0x00` | TIMER_LO | R | Counter low 32 bits (50 MHz) |
| `0x04` | TIMER_HI | R | Counter high 32 bits |
| `0x08` | CTRL | R/W | Bit 0: enable, Bit 1: auto-reload |
| `0x0C` | CMP_LO | R/W | Compare low (interrupt when counter >= compare) |
| `0x10` | CMP_HI | R/W | Compare high |

### VGA (`0x0400_3000`)

| Offset | Register | Access | Description |
|--------|----------|--------|-------------|
| `0x00` | FB_ADDR | R/W | Write address (0..63999) |
| `0x04` | FB_DATA | W | Pixel color (RGB332), auto-increments FB_ADDR |
| `0x08` | FB_STATUS | R | Bit 0: in vertical blanking |

RGB332 format: `[7:5] = R`, `[4:2] = G`, `[1:0] = B`.
Resolution: 320x200, upscaled 2x to 640x400 (letterboxed in 640x480 VGA output).

### GPIO (`0x0400_1000`)

See `doc/GPIO.md`.

---

## 7. Writing a Minimal BRAM Program

```c
// hello.c
#include "libs/uart.h"

#define BAUD_DIV_115200 27

void main(void) {
    uart_set_baud(BAUD_DIV_115200);
    uart_puts("Hello from Z-Core!\r\n");
    while (1);
}
```

Build with: `make APP=1 hello.bin`
Upload with: `python3 upload.py /dev/ttyUSB0 hello.bin`

---

## 8. Writing an SDRAM Program

For programs that need more than 12 KB, target SDRAM with a custom linker script:

```c
// big_program.c — runs from SDRAM
#include "libs/uart.h"

void main(void) {
    uart_set_baud(27);
    uart_puts("Running from SDRAM!\r\n");

    // Can use the full SDRAM address space
    volatile unsigned int *sdram = (volatile unsigned int *)0x10100000;
    sdram[0] = 0xDEADBEEF;
    // ...

    while (1);
}
```

Use a linker script that places `.text` at `0x10000000` (see `software/doom_riscv/src/riscv/zcore.lds` for an example).

Upload with: `python3 upload.py /dev/ttyUSB0 big_program.bin --base 0x10000000`


## 9. Playing DOOM

DOOM on Z-Core renders to the VGA display (320×200) and communicates over UART. Because the UART is shared for both debug output and input, a companion Python script — `doom_input.py` — handles keyboard input in a second terminal.

### Step-by-Step

**Terminal 1 — Upload and monitor:**

```bash
cd software/

# Recommended: use --fast to upload at 460800 baud (~1.5 min instead of ~7 min)
python3 upload.py /dev/ttyUSB0 \
    --segments doom_riscv/src/riscv/doom-zcore.bin@0x10000000 \
               doom_riscv/doom1.wad@0x12010000 \
    --entry 0x10000000 \
    --fast
```

Wait for the bootloader to finish uploading and print `Jump 10000000`. DOOM will then start, and the title screen will appear on the VGA monitor. This terminal will continue printing FPS and performance counters every 100 frames.

**Terminal 2 — Start the input driver:**

```bash
python3 software/doom_riscv/doom_input.py /dev/ttyUSB0
# Or at 460800 if you used --fast above:
python3 software/doom_riscv/doom_input.py /dev/ttyUSB0 --baud 460800
```

The input driver puts your terminal into raw mode and forwards keypresses to DOOM. Press `q` or `Ctrl-C` to exit the input driver.

### Controls

| Key | Action |
|-----|--------|
| `W` / `↑` | Move forward |
| `S` / `↓` | Move backward |
| `A` / `←` | Turn left |
| `D` / `→` | Turn right |
| `z` | Fire |
| `Space` | Use / open door |
| `x` | Run (hold) |
| `1` – `7` | Select weapon |
| `Tab` | Toggle automap |
| `Escape` | Open/close menu |
| `Enter` | Confirm menu selection |
| `p` | Pause |
| `+` / `-` | Adjust gamma |
| `F1` – `F12` | Function keys (save, load, etc.) |
| `q` | Quit `doom_input.py` (does not exit DOOM) |

> **Tip:** Arrow keys also work as listed, both in the terminal (escape sequences) and via the WASD bindings.

### UART Output

While DOOM runs, Terminal 1 prints a performance summary every 100 frames:

```
[FPS] 14
[Cycle Count] 357142857
[Instret Count] 178571428
[CPI] 2
```

- **FPS** — rendered frames per second.
- **CPI** — cycles per retired instruction (lower is better).

### WAD File Notes

`doom1.wad` is not included in this repository. See Section 5 ("Example: DOOM") for how to obtain it legally (DOOM shareware v1.9 or FreeDOOM).

Place `doom1.wad` at `software/doom_riscv/doom1.wad` before uploading. The bootloader loads it directly from that path into SDRAM at `0x1201_0000`.
