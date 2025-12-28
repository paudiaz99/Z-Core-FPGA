# Z-Core FPGA Deployment Guide

A comprehensive guide for compiling, linking, and deploying RV32IM software on the Z-Core processor implemented on the DE10-Lite FPGA development board.

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Prerequisites](#prerequisites)
3. [Software Toolchain](#software-toolchain)
4. [The Linker: Memory Layout and Executable Generation](#the-linker-memory-layout-and-executable-generation)
5. [HEX File Format and Memory Initialization](#hex-file-format-and-memory-initialization)
6. [Compilation Workflow](#compilation-workflow)
7. [FPGA Synthesis and Deployment](#fpga-synthesis-and-deployment)
8. [Troubleshooting](#troubleshooting)
9. [References](#references)

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
│  │    (RV32IM)  │────>│    Interconnect   │                     │
│  │              │     │                   │                     │
│  └──────────────┘     └───────┬───────────┘                     │
│                               │                                 │
│            ┌──────────────────┼──────────────────┐              │
│            ▼                  ▼                  ▼              │
│    ┌──────────────┐   ┌──────────────┐   ┌──────────────┐       │
│    │   Block RAM  │   │     UART     │   │     GPIO     │       │
│    │   (4 KB)     │   │  (9600 baud) │   │  (64 pins)   │       │
│    └──────────────┘   └──────────────┘   └──────────────┘       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Memory Map

| Address Range              | Size   | Peripheral | Description                       |
|----------------------------|--------|------------|-----------------------------------|
| `0x0000_0000 - 0x0000_0FFF`| 4 KB   | Block RAM  | Program memory (code + data)      |
| `0x0400_0000 - 0x0400_0FFF`| 4 KB   | UART       | Serial communication              |
| `0x0400_1000 - 0x0400_1FFF`| 4 KB   | GPIO       | General-purpose I/O               |

> [!IMPORTANT]
> **Current Memory Limitation**: The system currently uses **4 KB (4096 bytes)** of on-chip block RAM for program memory. This is *not* the FPGA's external SDRAM (64 MB). The block RAM is configured with `ADDR_WIDTH=12` bits, yielding 2^10 = 1024 addressable 32-bit words.

### Clocking

- **Input Clock**: `MAX10_CLK1_50` — 50 MHz oscillator on DE10-Lite
- **CPU Clock**: 50 MHz (synchronous, no PLL division)
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
| **RISC-V GNU Toolchain** | Cross-compilation for RV32IM           | [riscv-gnu-toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain) |
| **Python 3.x**           | ELF-to-HEX conversion script           | [python.org](https://www.python.org/)     |
| **Intel Quartus Prime**  | FPGA synthesis and programming         | [Intel FPGA](https://www.intel.com/fpga)  |
| **PowerShell** or **Bash** | Build script execution               | Pre-installed on Windows/Linux            |

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
| `-march=rv32i`   | Target ISA: RV32I (32-bit base integer)               |
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
    RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 4K
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

## HEX File Format and Memory Initialization

After linking, the ELF executable must be converted to a format that Verilog's `$readmemh` can understand for block RAM initialization.

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

## Compilation Workflow

### Manual Compilation Steps

```bash
# 1. Navigate to software directory
cd software

# 2. Assemble startup code
riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 -c start.S -o start.o

# 3. Compile C source
riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 -O2 \
    -ffreestanding -nostdlib -c hello.c -o hello.o

# 4. Link into ELF executable
riscv64-unknown-elf-ld -T linker.ld -Map=hello.map hello.o start.o -o hello.elf

# 5. Generate disassembly listing (optional, for debugging)
riscv64-unknown-elf-objdump -d -S hello.elf > hello.lst

# 6. Convert to HEX
python elf2hex.py hello.elf hello.hex

# 7. Verify size (must fit in 4 KB = 1024 words)
riscv64-unknown-elf-size hello.elf
```

### Using the Build Script (Bash)

```bash
cd software
./hex_gen.sh -Target hello
```

### Using the Makefile

```bash
cd software
make              # Build all programs
make clean        # Remove build artifacts
```

### Verifying the Build

After compilation, check that the binary fits within the 4 KB memory:

```bash
$ riscv64-unknown-elf-size hello.elf
   text    data     bss     dec     hex filename
    720       0       0     720     2d0 hello.elf
```

> [!CAUTION]
> If `text + data + bss > 4096 bytes`, the program will not fit in block RAM and will cause unpredictable behavior. Either reduce program size or increase `ADDR_WIDTH` in `axil_ram` instantiation.

---

## FPGA Synthesis and Deployment

### Synthesis Flow

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│  Verilog RTL    │ ──▶ │  Analysis &     │ ──▶ │    Fitter       │
│  + HEX File     │     │  Synthesis      │     │  (Place & Route)│
└─────────────────┘     └─────────────────┘     └─────────────────┘
                                                        │
                                                        ▼
                        ┌─────────────────┐     ┌─────────────────┐
                        │  Program FPGA   │ ◀── │  Timing Analysis│
                        │  (.sof/.pof)    │     │  (.sdc)         │
                        └─────────────────┘     └─────────────────┘
```

### Step-by-Step Deployment

1. **Compile Software**
   ```bash
   cd software
   ./hex_gen.sh -Target hello
   ```

2. **Verify HEX File Exists**
   ```bash
   ls software/*.hex
   ```

3. **Open Quartus Project**

4. **Set Memory Initialization (if needed)**
   - Edit `z_core_top_model.v` parameter:
     ```verilog
     parameter INIT_FILE = "software/hello.hex"
     ```

5. **Run Analysis & Synthesis**
   - Processing → Start → Analysis & Synthesis
   - Or: `Ctrl + K`

6. **Run Fitter**
   - Processing → Start → Fitter

7. **Run Timing Analysis**
   - Verify all timing constraints are met (no negative slack)

8. **Program FPGA**
   - Tools → Programmer
   - Select `output_files/Z-Core.sof`
   - Click "Start"

9. **Verify Operation**
   - Press and release `KEY[0]` to reset
   - Observe `LEDR[9]` blinking (heartbeat)
   - Observe `LEDR[7:0]` for GPIO activity

### Important Quartus Settings

| Setting                  | Value/Location                              |
|--------------------------|---------------------------------------------|
| Timing Constraints       | `Z-Core.sdc` (must be in project)           |
| Device                   | MAX 10 (10M50DAF484C7G)                     |
| Top-Level Entity         | `z_core_top`                                |
| Memory Initialization    | Automatic via `$readmemh` and `INIT_FILE`   |

---

## Troubleshooting

### Common Issues

| Symptom                           | Likely Cause                                    | Solution                                      |
|-----------------------------------|------------------------------------------------|-----------------------------------------------|
| No heartbeat on LEDR[9]           | Clock not applied or reset stuck               | Check KEY[0], verify clock source             |
| Program doesn't run               | HEX file missing or path incorrect             | Verify file exists; re-run synthesis          |
| Random LED behavior               | BSS not cleared, or uninitialized variables    | Ensure `start.S` is linked first              |
| "1 GHz" timing error              | Missing SDC file                               | Add `Z-Core.sdc` to project                   |
| HEX file too large                | Program exceeds 4 KB memory                    | Optimize code or increase RAM size            |
| Linker error: undefined `_start`  | Startup code not assembled                     | Compile `start.S` first                       |
| Objcopy fails                     | RISC-V toolchain not in PATH                   | Add toolchain `bin/` to system PATH           |

### Debugging Techniques

1. **Check Disassembly**
   ```bash
   cat hello.lst | head -50
   ```

2. **Verify HEX Content**
   ```bash
   head -10 hello.hex
   ```
   First instruction should be `lui sp, 0x1...` or similar startup code.

3. **Check Map File for Addresses**
   ```bash
   cat hello.map | grep -A5 ".text"
   ```

4. **Simulate in ModelSim**
   
   Create a testbench `top_module_tb.v` with the HEX file for pre-synthesis verification.

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
