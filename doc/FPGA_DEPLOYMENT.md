# Z-Core FPGA Deployment Guide

A comprehensive guide for compiling, linking, and deploying RV32IM software on the Z-Core processor implemented on the DE10-Lite FPGA development board.

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Prerequisites](#prerequisites)
3. [Software Toolchain](#software-toolchain)
4. [The Linker: Memory Layout and Executable Generation](#the-linker-memory-layout-and-executable-generation)
5. [The Bootloader Flow (Fast)](#the-bootloader-flow-fast)
6. [The Bootloader Update Flow (Slow)](#the-bootloader-update-flow-slow)
7. [Troubleshooting](#troubleshooting)
8. [References](#references)

---

## System Overview

The Z-Core is a pipelined RISC-V RV32IM processor designed for educational purposes and FPGA deployment. It features an AXI-Lite bus interface connecting the CPU to memory and peripherals.

### Hardware Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         Z-Core Top Module                       │
│                                                                 │
│  ┌──────────────┐     ┌───────────────────┐                     │
│  │    Z-Core    │     │      AXI-Lite     │                     │
│  │ (RV32IMZicsr)│────>│    Interconnect   │                     │
│  │              │     │                   │                     │
│  └──────────────┘     └───────┬───────────┘                     │
│                               │                                 │
│         ┌─────────────┬───────┴───────┬─────────────┐           │
│         ▼             ▼               ▼             ▼           │
│  ┌────────────┐ ┌────────────┐  ┌────────────┐ ┌────────────┐   │
│  │ Block RAM  │ │    UART    │  │    GPIO    │ │ VGA / Timer│   │
│  │  (16 KB)   │ │(115200 bd) │  │  (16 pins) │ │(Peripherals)│   │
│  └────────────┘ └────────────┘  └────────────┘ └────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Memory Map

| Address Range              | Size   | Peripheral | Description                       |
|----------------------------|--------|------------|-----------------------------------|
| `0x0000_0000 - 0x0000_0FFF`| 4 KB   | Block RAM  | Bootloader (MIF initialization)   |
| `0x0000_1000 - 0x0000_3FFF`| 12 KB  | Block RAM  | Application Space (UART upload)   |
| `0x0400_0000 - 0x0400_0FFF`| 4 KB   | UART       | Serial communication              |
| `0x0400_1000 - 0x0400_1FFF`| 4 KB   | GPIO       | General-purpose I/O               |
| `0x0400_2000 - 0x0400_2FFF`| 4 KB   | Timer      | 64-bit Timer/Counter              |
| `0x0400_3000 - 0x0400_3FFF`| 4 KB   | VGA        | 160x120 VGA Controller            |

> [!IMPORTANT]
> **Memory Segmentation**: The 16 KB of on-chip RAM is split into two regions:
> 1. **Bootloader**: The first 4 KB (`0x0000-0x0FFF`) contain the fixed bootloader code.
> 2. **Application**: The remaining 12 KB (`0x1000-0x3FFF`) are available for user programs.

### Clocking

- **Input Clock**: `MAX10_CLK1_50` — 50 MHz oscillator on DE10-Lite
- **CPU Clock**: 50 MHz
- **UART Baud**: 115200 (for bootloader and user apps)
- **Timing Constraints**: Defined in `Z-Core.sdc` (20 ns period constraint)

### Reset Signal

| Signal | Active Level | Control Source | Description                     |
|--------|--------------|----------------|---------------------------------|
| `rstn` | Low          | `KEY[0]`       | Asynchronous system reset      |

- **Pressed** (Logic 0): Reset held active — all registers cleared
- **Released** (Logic 1): System runs normally

### LED Debug Indicators

| LED        | Signal Description                          |
|------------|---------------------------------------------|
| `LEDR[7:0]`| GPIO output register (directly visible)     |
| `LEDR[8]`  | AXI-Lite AR channel ready (read activity)   |
| `LEDR[9]`  | Heartbeat indicator (~0.74 Hz, bit 25)      |

---

## Prerequisites

Before deploying code to Z-Core, ensure the following are installed and accessible from your system PATH:

### Required Software

| Tool                     | Purpose                                | Installation                              |
|--------------------------|----------------------------------------|-------------------------------------------|
| **RISC-V GNU Toolchain** | Cross-compilation for RV32IMZicsr      | [riscv-gnu-toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain) |
| **Python 3.x**           | Bootloader upload and MIF generation   | [python.org](https://www.python.org/)     |
| **Intel Quartus Prime**  | FPGA synthesis and programming         | [Intel FPGA](https://www.intel.com/fpga)  |

### RISC-V Toolchain Configuration

The toolchain must be built for the **RV32IM** integer instruction set with multiply/divide extension:

```bash
# Example build configuration for rv32im
./configure --prefix=/opt/riscv --with-arch=rv32im --with-abi=ilp32
make
```

Verify installation:

```bash
riscv64-unknown-elf-gcc --version
riscv64-unknown-elf-objcopy --version
```

> [!NOTE]
> Despite the `riscv64` prefix, the toolchain supports 32-bit targets when using `-march=rv32im -mabi=ilp32` flags.

---

## Software Toolchain

### Toolchain Components

| Tool                        | Purpose                                                  |
|-----------------------------|----------------------------------------------------------|
| `riscv64-unknown-elf-gcc`   | C compiler with integrated assembler                     |
| `riscv64-unknown-elf-ld`    | GNU Linker — combines object files into executable       |
| `riscv64-unknown-elf-objcopy`| Converts ELF to raw binary or other formats            |
| `riscv64-unknown-elf-objdump`| Disassembler for debugging and verification            |
| `riscv64-unknown-elf-size`  | Reports section sizes in compiled binaries              |

### Compiler Flags Reference

| Flag             | Description                                           |
|------------------|-------------------------------------------------------|
| `-march=rv32im_zicsr` | Target ISA: RV32IM + Zicsr (32-bit integer + Mul/Div + CSR) |
| `-mabi=ilp32`    | ABI: 32-bit integers, longs, and pointers             |
| `-O2`            | Optimization level 2 (recommended for size/speed)     |
| `-nostartfiles`  | Do not link standard startup files (crt0, etc.)       |
| `-ffreestanding` | Freestanding environment (no OS, no standard library) |
| `-nostdlib`      | Do not link standard C library                        |
| `-T linker.ld`   | Use custom linker script for memory layout            |

---

## The Linker: Memory Layout and Executable Generation

The **GNU Linker** (`ld`) is responsible for combining compiled object files (`.o`) into a single executable (`.elf`). It determines where each section of code and data is placed in memory according to a **linker script**.

### What the Linker Does

1. **Symbol Resolution**: Resolves references between object files (e.g., function calls)
2. **Section Merging**: Combines `.text`, `.data`, `.bss` sections from multiple files
3. **Address Assignment**: Assigns absolute memory addresses based on linker script
4. **Relocation**: Adjusts addresses in instructions to reflect final memory layout
5. **Entry Point Definition**: Sets the program's starting address

### The Linker Script (`linker.ld`)

The linker script defines the memory regions and section placement:

```ld
OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x00001000, LENGTH = 12K
}

SECTIONS
{
    . = 0x00000000;
    
    .text : {
        *(.text.start)     /* Startup code first */
        *(.text*)          /* All other code */
        *(.rodata*)        /* Read-only data (constants, strings) */
    } > RAM
    
    .data : {
        __data_start = .;
        *(.data*)          /* Initialized global/static variables */
        __data_end = .;
    } > RAM
    
    .bss : {
        __bss_start = .;
        *(.bss*)           /* Uninitialized global/static variables */
        *(COMMON)          /* Common symbols */
        __bss_end = .;
    } > RAM
    
    . = ALIGN(8);
    _end = .;
    
    /* Stack grows down from top of addressable space */
    _stack_top = 0x00001000;
}
```

### Linker Script Key Concepts

| Directive           | Purpose                                                      |
|---------------------|--------------------------------------------------------------|
| `OUTPUT_ARCH`       | Specifies target architecture (RISC-V)                       |
| `ENTRY(_start)`     | Defines program entry point symbol                           |
| `MEMORY { ... }`    | Declares available memory regions with attributes            |
| `SECTIONS { ... }`  | Controls placement of input sections into memory regions     |
| `. = 0x00000000`    | Sets location counter (current address)                      |
| `*(.text*)`         | Matches all `.text` sections from all input files            |
| `> RAM`             | Places section into the RAM memory region                    |
| `__bss_start = .`   | Creates symbol at current address (for runtime BSS clearing) |

### Section Descriptions

| Section   | Content                               | Initialized | Loaded to ROM |
|-----------|---------------------------------------|-------------|---------------|
| `.text`   | Executable machine code               | Yes         | Yes           |
| `.rodata` | Read-only constants, string literals  | Yes         | Yes           |
| `.data`   | Initialized global/static variables   | Yes         | Yes           |
| `.bss`    | Uninitialized global/static variables | No (zeroed) | No            |


#### Runtime Memory Layout

```
┌─────────────────────────────────────┐  0x00000000 (Reset Vector)
│               .text                 │  ← Machine code (instructions)
│      (executable code)              │     Placed FIRST by linker
│                                     │
├─────────────────────────────────────┤  (end of .text)
│               .rodata               │  ← Read-only data
│  (constant strings, lookup tables)  │     Merged into .text section
│                                     │
├─────────────────────────────────────┤  (end of .rodata)
│               .data                 │  ← Initialized global variables
│     (pre-initialized globals)       │     Placed AFTER code
│                                     │
├─────────────────────────────────────┤  (end of .data)
│               .bss                  │  ← Uninitialized globals
│   (zero-initialized at startup)     │     Placed AFTER .data
│                                     │
├─────────────────────────────────────┤  _end symbol
│                                     │
│           (Free Memory)             │  ← Available for heap
│                                     │     (if dynamic allocation used)
│                                     │
├─────────────────────────────────────┤
│                                     │
│               Stack                 │  ← Stack grows DOWNWARD
│                                     │     from _stack_top
│                                     │
└─────────────────────────────────────┘  _stack_top (e.g., 0x00001000)
```

#### The Location Counter Mechanism

The linker uses a **location counter** (`.`) to track the current memory address during linking:

```ld
. = 0x00000000;           // Location counter starts at 0

.text : {                 // .text section placed at current location (0)
    *(.text.start)        // Startup code first (reset vector)
    *(.text*)             // All other code follows sequentially
    *(.rodata*)           // Read-only data appended
}                         // Location counter now = 0 + sizeof(.text)

.data : {                 // .data placed at NEW location counter value
    *(.data*)             // No overlap with .text — guaranteed!
}                         // Location counter now += sizeof(.data)

.bss : {                  // .bss placed after .data
    *(.bss*)
}

_end = .;                 // Symbol marks end of used memory
```

#### Example: Section Layout for `hello.elf`

```
$ riscv64-unknown-elf-size -A hello.elf

hello.elf  :
section              size      addr
.text                 720   0x00000000
.rodata                48   0x000002D0
.data                   0   0x00000300
.bss                    0   0x00000300
Total                 768
```

In this example:
- `.text` occupies addresses `0x000 - 0x2CF` (720 bytes)
- `.rodata` occupies addresses `0x2D0 - 0x2FF` (48 bytes)
- `.data` and `.bss` are empty (no global variables)
- `_end` = `0x300` (768 bytes used)
- Free memory: `0x300` to `_stack_top`

#### Stack vs. Code/Data: The Safety Margin

The stack and static data occupy **opposite ends** of memory:

```
                    Code/Data grow UP (during linking)
                              ↑
┌────────────────────────────────────────────────────────┐
│  .text  │  .data  │  .bss  │ ← _end    FREE    stack   │
└────────────────────────────────────────────────────────┘
0x000                                              _stack_top
                                                        ↓
                                    Stack grows DOWN (at runtime)
```

**Safety Condition**: No collision occurs as long as:
```
_end + max_stack_usage < _stack_top
```

### Startup Code (`start.S`)

The startup assembly code initializes the runtime environment before `main()`:

```asm
.section .text.start
.global _start

_start:
    # Initialize stack pointer
    lui sp, %hi(_stack_top)
    addi sp, sp, %lo(_stack_top)
    
    # Clear BSS section (required for zero-initialized variables)
    la a0, __bss_start
    la a1, __bss_end
    j 2f
1:
    sw zero, 0(a0)
    addi a0, a0, 4
2:
    blt a0, a1, 1b
    
    # Call main function
    call main
    
    # Infinite loop if main returns
_loop:
    j _loop
```

> [!IMPORTANT]
> The startup code is placed in `.text.start` section, which the linker script positions at address `0x00000000` — the CPU's reset vector. The CPU begins execution from this address after reset.

---

## Memory Initialization (MIF)

The processor's instruction memory is initialized during FPGA synthesis. While applications are uploaded via UART, the initial bootloader is baked into the bitstream using a Memory Initialization File (`.mif`).

### Compilation to HEX Pipeline

```
┌────────────┐      ┌────────────┐      ┌────────────┐
│  C Source  │ ───> │ Object File│ ───> │    ELF     │
│  (.c, .S)  │      │   (.o)     │      │  (.elf)    │
└────────────┘      └────────────┘      └────────────┘
                         │GCC                │ GCC/LD
                         ▼                   ▼
                   ┌────────────────────────────────┐
                   │          elf2hex.py            │
                   │  (or objcopy -O verilog)       │
                   └────────────────────────────────┘
                                  │
                                  ▼
                         ┌────────────────┐
                         │   HEX File     │
                         │  (32-bit words)│
                         └────────────────┘
                                  │
                                  ▼
                         ┌────────────────┐
                         │  Block RAM     │
                         │  ($readmemh)   │
                         └────────────────┘
```

### HEX File Format

The HEX file contains one 32-bit word per line in uppercase hexadecimal:

```hex
00001137    // lui sp, 0x1          (offset 0x00) -> loads 0x1000
00010113    // addi sp, sp, 0       (offset 0x04)
2CA00513    // li a0, 0x2CA         (offset 0x08)
...
```

Each line represents one 32-bit instruction or data word at consecutive word addresses (0, 1, 2, ...), which correspond to byte addresses 0x00, 0x04, 0x08, etc.

### ELF to HEX Conversion Tool (`elf2hex.py`)

The custom Python script performs the following:

1. Calls `objcopy` to extract raw binary from ELF
2. Pads binary to 4-byte boundary
3. Interprets bytes as little-endian 32-bit words (RISC-V byte order)
4. Outputs one word per line in hex format

```python
# Core conversion logic
with open(bin_path, 'rb') as f:
    binary_data = f.read()

# Pad to word boundary
while len(binary_data) % 4 != 0:
    binary_data += b'\x00'

# Convert to 32-bit words (little-endian)
words = struct.unpack(f'<{num_words}I', binary_data)

# Write hex file
for word in words:
    f.write(f'{word:08X}\n')
```

### Verilog Memory Initialization

The block RAM module loads the HEX file during synthesis:

```verilog
// In axil_ram.v
parameter INIT_FILE = "";

initial begin
    if (INIT_FILE != "") begin
        $readmemh(INIT_FILE, mem);
    end
end
```

The top module specifies the initialization file:

```verilog
// In z_core_top_model.v
parameter INIT_FILE = "software/hello.hex"

axil_ram #(
    .INIT_FILE(INIT_FILE),
    ...
) u_memory ( ... );
```

> [!WARNING]
> The HEX file path is relative to the Quartus project directory. Ensure the path is correct, or synthesis will proceed with uninitialized (zero-filled) memory, causing undefined CPU behavior.

---

## Deployment Flow: Bootloader vs Hardware

Z-Core uses a bootloader-based deployment strategy to minimize the need for slow FPGA recompilations.

| Feature | Bootloader (Application) | Hardware (Bootloader/SoC) |
|---------|--------------------------|---------------------------|
| **Target Address** | `0x1000` | `0x0000` |
| **Build Command**  | `make APP=1 myapp.bin` | `cd bootloader/ && make` |
| **Deployment**     | `python3 upload.py` (UART) | Quartus Recompile + JTAG |
| **Typical Speed**  | ~5 Seconds | ~5 Minutes |

---

## The Bootloader Flow (Fast)

This is the primary workflow for developing software. You only go through Quartus once to flash the initial bitstream; after that, all application changes happen over UART.

### Step 1 — Compile
Navigate to the `software/` directory and compile your code:
```bash
cd software/
make APP=1 hello.bin
```
*   `APP=1` uses `linker_app.ld`, which places your code at `0x1000` (the application space).
*   Replace `hello` with your filename (no extension).

### Step 2 — Upload
```bash
python3 upload.py /dev/ttyUSB0 hello.bin
```
This sends the binary to the bootloader over UART at 115200 baud. The bootloader writes it to RAM starting at `0x1000` and jumps to it automatically.

*Check your port with `ls /dev/ttyUSB*` if unsure.*

### Step 3 — Monitor
The upload script enters terminal mode automatically after a successful upload.
*   **Interact**: Typed characters are sent to the FPGA.
*   **Exit**: Press **Ctrl+C**.
*   **Skip**: Use `python3 upload.py /dev/ttyUSB0 hello.bin -n` to upload only.

> [!IMPORTANT]
> **Memory Limits**: The application space is **12 KB** (`0x1000`–`0x3FFF`). Ensure your total binary size (text + data + bss) stays under ~12,000 bytes. Use `riscv32-unknown-elf-size myapp.elf` to check.

---

## The Bootloader Update Flow (Slow)

The bootloader lives at address `0x0000` and is baked directly into the FPGA bitstream as initialized M9K RAM content (the `.mif` file). You only need to redo this if the bootloader code or the hardware peripherals change.

### Step 1 — Compile the bootloader
```bash
cd software/bootloader/
make
```
This produces `bootloader.mif` in the `software/` directory. Quartus reads this file during synthesis to initialize the memory.

### Step 2 — Recompile the FPGA bitstream
1. Open your project in **Quartus Prime**.
2. Run **Start Compilation** (`Processing → Start Compilation` or **Ctrl+L**).
3. This process embeds the new `.mif` into the `.sof` bitstream.

### Step 3 — Program the FPGA
1. Open the **Programmer** (`Tools → Programmer`).
2. Select `output_files/Z-Core.sof`.
3. Click **Start**. This flashes the bitstream (including the new bootloader) via JTAG.

---

## Troubleshooting

### Common Issues

| Symptom                           | Likely Cause                                    | Solution                                      |
|-----------------------------------|------------------------------------------------|-----------------------------------------------|
| No heartbeat on LEDR[9]           | Clock not applied or reset stuck               | Check KEY[0], verify clock source             |
| `upload.py` timeout/fail          | Incorrect port or bootloader not running       | Check `/dev/ttyUSBx`, press reset on FPGA     |
| Random LED/VGA behavior           | BSS not cleared, or stack overflow             | Ensure `start.S` is linked; check memory size |
| "1 GHz" timing error              | Missing SDC file                               | Add `Z-Core.sdc` to project                   |
| Binary too large (> 12 KB)        | Program exceeds application space              | Optimize code or use `-Os` optimization       |
| Linker error: undefined `_start`  | Startup code not included                      | Ensure `start.S` is compiled and linked       |
| `upload.py`: Permission denied    | User not in `dialout` or `uucp` group          | Run `sudo chmod 666 /dev/ttyUSB0`             |

### Debugging Techniques

1. **Check Disassembly**
   ```bash
   riscv32-unknown-elf-objdump -d myapp.elf | head -50
   ```
   Verify that the entry point is at `0x1000` (for applications) or `0x0000` (for bootloader).

2. **Verify Memory Map**
   ```bash
   cat myapp.map | grep -A10 "Memory Configuration"
   ```

3. **Check Section Sizes**
   ```bash
   riscv32-unknown-elf-size myapp.elf
   ```
   The sum of `text + data + bss` must be less than 12,288 bytes (12 KB).

4. **Verify UART Output**
   The `upload.py` script acts as a terminal. Use `uart_puts()` in your code to print debug messages to your PC console.

---

## References

### RISC-V Architecture

- [RISC-V Specification (Volume 1: Unprivileged ISA)](https://riscv.org/technical/specifications/)
- [RISC-V Assembly Programmer's Manual](https://github.com/riscv-non-isa/riscv-asm-manual)

### GNU Toolchain

- [RISC-V GNU Toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain)
- [GCC RISC-V Options](https://gcc.gnu.org/onlinedocs/gcc/RISC-V-Options.html)

### Intel FPGA

- [DE10-Lite User Manual](https://ftp.intel.com/Public/Pub/fpgaup/pub/Intel_Material/Boards/DE10-Lite/DE10_Lite_User_Manual.pdf)
- [MAX 10 FPGA Device Handbook](https://www.intel.com/programmable/technical-pdfs/max10-handbook.pdf)
