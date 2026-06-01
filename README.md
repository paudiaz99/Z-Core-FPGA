<div align="center">
<pre>
███████╗       ██████╗ ██████╗ ██████╗ ███████╗                            
╚══███╔╝      ██╔════╝██╔═══██╗██╔══██╗██╔════╝                            
  ███╔╝ █████╗██║     ██║   ██║██████╔╝█████╗     ___  ___   ___    _      
 ███╔╝  ╚════╝██║     ██║   ██║██╔══██╗██╔══╝    | __|| _ \ / __|  /_\     
███████╗      ╚██████╗╚██████╔╝██║  ██║███████╗  | _| |  _/| (_ | / _ \    
╚══════╝       ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝  |_|  |_|   \___|/_/ \_\   
</pre>
</div>

<div align="center">
  
**Z-Core FPGA Implementation for DE10-Lite Board**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![RISC-V](https://img.shields.io/badge/RISC--V-RV32IMZicsr-green?logo=riscv)
[![FPGA](https://img.shields.io/badge/FPGA-MAX%2010-0071c5.svg)](https://www.altera.com/asap/offering/a1jui0000049upbmam/max-10-device-family-de10-lite-board)
[![Board](https://img.shields.io/badge/Board-DE10--Lite-00a98f.svg)](https://www.terasic.com.tw/cgi-bin/page/archive.pl?Language=English&No=1021)


</div>


---

## Block Diagram

<div align="center">
  <img src="https://github.com/user-attachments/assets/98914226-4635-45d5-9400-1545cf6fb7ab" alt="centered image">
  <br>
  <sup>Z-Core SoC Architecture.</sup>
</div>

> **Note**: The Block diagram does not include the VGA controller.

## Z-Core RV32IMZicsr Architecture
<div align="center">
  <img src="https://github.com/user-attachments/assets/5d1e0237-2ab6-4d9c-bd10-bd1ab6458d75" alt="centered image">
  <br>
  <sup>Z-Core RV32IMZicsr Architecture Diagram.</sup>
</div>


> **Note**: For Z-Core detailed processor architecture explanation, pipeline implementation, verification methodology, and ISA compliance documentation, refer to the main **[Z-Core repository](https://github.com/paudiaz99/Z-Core)**.

---

## Overview

This repository contains the FPGA implementation of the Z-Core RISC-V RV32IMZicsr processor, targeting the Intel DE10-Lite development board (MAX 10 FPGA). The Core features a 32KB direct-mapped instruction cache, a 32KB 2-way set-associative data cache with write-back/write-allocate policy, a branch predictor, and a 64MB SDRAM interface, enabling large applications such as DOOM to run at playable framerates.

The system supports two execution modes: small programs run directly from on-chip Block RAM (16 KB), while large applications like DOOM are loaded into the 64MB SDRAM via a UART bootloader and execute from there with cache assistance.

---

<div align="center">
  <img src="https://github.com/user-attachments/assets/72eb1558-ab39-4f42-9cb9-ddd9adee80e5" alt="centered image">
  <br>
  <sup>Z-Core Running VGA Test @ 60 FPS.</sup>
</div>

---

### Key Specifications

| Parameter | Value |
|-----------|-------|
| Target FPGA | Intel MAX 10 (10M50DAF484C7G) |
| Operating Frequency | 50 MHz |
| ISA | RV32IM + Zicsr |
| Instruction Cache | 32 KB direct-mapped (8192 entries) |
| Data Cache | 32 KB 2-way set-associative (write-back, write-allocate) |
| Branch Predictor | 2-Bit Branch Predictor |
| BRAM | 16 KB on-chip (bootloader + small apps) |
| SDRAM | 64 MB IS42S16320D @ 100 MHz (via AXI-CDC bridge) |
| Peripherals | UART, GPIO (16-bit), VGA (320×200 RGB332), 64-bit Timer |
| Development Board | Terasic DE10-Lite |

---

## Performance

All measurements on DE10-Lite (MAX 10) at 50 MHz.

### CoreMark

| Metric | Value |
|--------|-------|
| CoreMark/MHz | **3.06** |

### DOOM (320×200, `doom1.wad` shareware)

| Configuration | FPS |
|---------------|-----|
| No cache (SDRAM only) | < 1 FPS |
| 32 KB Instruction Cache only | ~3 FPS |
| 32 KB I-Cache + 32 KB 2-way D-Cache | **~14 FPS** |
| 32 KB I-Cache + 32 KB 2-way D-Cache (lower resolution) | **~20 FPS** |

### Memory Bandwidth (STREAM benchmark)

| Source | Bandwidth |
|--------|-----------|
| SDRAM (cache miss path) | 4.15 – 4.66 MB/s |
| Data Cache (cache hit path) | 57 – 80 MB/s |

---

## Prerequisites

| Tool | Purpose |
|------|---------|
| Intel Quartus Prime Lite | FPGA synthesis and programming |
| RISC-V GNU Toolchain | Cross-compilation (`riscv32-unknown-elf-`, rv32im_zicsr target) |
| Python 3.x | ELF-to-HEX conversion and UART upload script |

---

## Quick Start (UART Bootloader)

The Z-Core SoC includes a UART bootloader stored in Block RAM, allowing you to upload and run applications without recompiling the FPGA hardware.

### Small programs (BRAM, ≤ 12 KB)

```bash
cd software/
make APP=1 hello.bin
python3 upload.py /dev/ttyUSB0 hello.bin
```

### DOOM (SDRAM, ~4.5 MB payload)

```bash
# Build DOOM binary
cd software/Z-Core-DOOM/src/riscv/
make all

# Upload DOOM code + WAD (two segments to SDRAM)
cd software/
python3 upload.py /dev/ttyUSB0 \
    --segments Z-Core-DOOM/src/riscv/doom-zcore.bin@0x10000000 \
               Z-Core-DOOM/doom1.wad@0x12010000 \
    --entry 0x10000000
```

> For the complete guide — WAD file sourcing, baud-rate speedup (`--fast`), controls, and playing DOOM — see **[doc/SOFTWARE.md § 10](doc/SOFTWARE.md#10-playing-doom)**.

### Quick Reference

| What | Command |
|------|---------|
| Build (BRAM) | `make APP=1 myprogram.bin` |
| Upload + Monitor | `python3 upload.py /dev/ttyUSB0 myprogram.bin` |
| Upload only | `python3 upload.py /dev/ttyUSB0 myprogram.bin -n` |
| Clean build | `make clean` |
| Build DOOM | `cd Z-Core-DOOM/src/riscv && make all` |

---

## Software Examples

The `software/` directory contains examples compiled with the RISC-V GNU Toolchain.

| Program | Description |
|---------|-------------|
| `hello` | "Hello, World!" via UART |
| `led_test` | Blink the 10 LEDs on DE10-Lite |
| `gpio_test` | GPIO read/write test |
| `vga_test` | VGA color bars and bouncing square |
| `space` | "Star Assault" VGA space shooter |
| `pong` | Classic Pong via UART |
| `multiplication` | RV32IM multiply/divide instruction test |
| `sdram_test` | SDRAM read/write/byte-strobe test |
| `Z-Core-DOOM/` | DOOM port for Z-Core (runs from SDRAM) |

### Pong Game

<div align="center">
  <img src="https://github.com/user-attachments/assets/b50e7727-6a6f-46b3-bfb0-b8266959b0bc" alt="centered image">
  <br>
  <sup>Pong Game via UART.</sup>
</div>

**Input GPIO Connection**

| GPIO | Function |
|------|----------|
| 8 | Paddle 1 Up |
| 9 | Paddle 1 Down |

> Connect two push buttons with pull-down resistors to GPIO[8] and GPIO[9].

---

## Memory Map

| Address Range | Peripheral | Size |
|---------------|------------|------|
| `0x0000_0000` – `0x0000_0FFF` | Bootloader (BRAM) | 4 KB |
| `0x0000_1000` – `0x0000_3FFF` | Application space (BRAM) | 12 KB |
| `0x0400_0000` – `0x0400_0FFF` | UART | 4 KB |
| `0x0400_1000` – `0x0400_1FFF` | GPIO | 4 KB |
| `0x0400_2000` – `0x0400_2FFF` | Timer | 4 KB |
| `0x0400_3000` – `0x0400_3FFF` | VGA | 4 KB |
| `0x1000_0000` – `0x13FF_FFFF` | SDRAM (64 MB) | 64 MB |

### BRAM Layout

```
0x0000_0000 +------------------+
            | Bootloader (4 KB)|  ← baked into bitstream from MIF
0x0000_1000 +------------------+
            | App space (12 KB)|  ← loaded by bootloader over UART
0x0000_4000 +------------------+
            | (stack top)      |
```

### SDRAM Layout (for DOOM and large programs)

```
0x1000_0000 +------------------+
            | .text / .rodata  |  Code + read-only data
            | .data / .bss     |  Initialized and zero data
            | heap →           |  Grows upward from _heap_start
            |                  |
            |         ← stack  |  Grows down from 0x11FF_0000
0x11FF_0000 +------------------+  Stack top (64 KB stack)
            | (guard region)   |
0x1201_0000 +------------------+
            | WAD / assets     |  doom1.wad loaded by bootloader
0x1241_0000 +------------------+
```

---

## Project Structure

```
├── rtl/                            # Synthesizable RTL
│   ├── z_core_top_model.v         # Top-level FPGA wrapper
│   ├── z_core_control_u.v         # CPU Control Unit (pipeline)
│   ├── z_core_alu.v               # Arithmetic Logic Unit
│   ├── z_core_alu_ctrl.v          # ALU Control Unit
│   ├── z_core_decoder.v           # Instruction Decoder
│   ├── z_core_reg_file.v          # General Purpose Registers
│   ├── z_core_csr_file.v          # CSR File (Zicsr extension)
│   ├── z_core_instr_cache.v       # 32 KB Direct-Mapped I-Cache
│   ├── z_core_data_cache.v        # 32 KB 2-Way Set-Assoc D-Cache
│   ├── z_core_branch_pred.v       # Branch Predictor
│   ├── z_core_pma.v               # Physical Memory Attributes checker
│   ├── z_core_pma_map.vh          # PMA region table (platform-specific)
│   ├── z_core_mult_unit.v         # Multiplier Unit
│   ├── z_core_div_unit.v          # Division Unit
│   ├── z_core_32b_timer.v         # 64-bit Timer core
│   ├── axil_interconnect.v        # AXI-Lite Bus Interconnect
│   ├── axil_timer.v               # Timer Peripheral
│   ├── axil_vga.v                 # VGA Controller Peripheral
│   ├── axil_uart.v                # UART Peripheral
│   ├── axil_gpio.v                # GPIO Peripheral
│   ├── axil_master.v              # AXI-Lite Master Interface
│   ├── axil_cdc_bridge.v          # AXI Clock-Domain Crossing Bridge
│   ├── axi_mem.v                  # AXI-Lite RAM Interface
│   ├── axi_sdram_bridge.v         # AXI-to-SDRAM Bridge
│   ├── sdram_axi_top.v            # SDRAM AXI Top (PLL + controller)
│   ├── sdram_pll0.v               # 100 MHz PLL for SDRAM
│   ├── sdram_pll0.qip             # Quartus PLL IP
│   ├── Sdram_Control/             # SDRAM Controller (IS42S16320D)
│   │   ├── Sdram_Control.v
│   │   ├── control_interface.v
│   │   ├── command.v
│   │   ├── sdr_data_path.v
│   │   ├── Sdram_WR_FIFO.v / .qip
│   │   ├── Sdram_RD_FIFO.v / .qip
│   │   └── Sdram_Params.h
│   ├── arbiter.v                  # AXI arbiter
│   ├── priority_encoder.v         # Priority encoder
│   └── flist.vc                   # RTL file list
│
├── software/                       # Programs and tools
│   ├── bootloader/                # On-chip UART bootloader
│   │   ├── bootloader.c           # Bootloader source (v3.1)
│   │   ├── boot_start.S           # Reset vector + trap handler
│   │   ├── Makefile
│   │   └── linker_boot.ld
│   ├── Z-Core-DOOM/               # DOOM port for Z-Core (submodule)
│   │   ├── src/riscv/             # Z-Core-specific DOOM backend
│   │   │   ├── zcore.lds          # Linker script (SDRAM target)
│   │   │   ├── config.h           # Platform addresses
│   │   │   ├── start.S            # DOOM startup
│   │   │   └── ...                # Platform driver files
│   │   └── doom1.wad              # (not included — see SOFTWARE.md)
│   ├── sdram_test/                # Standalone SDRAM test (SDRAM execution)
│   ├── libs/                      # Shared libraries
│   │   ├── uart.c / uart.h        # UART driver
│   │   ├── vga.h                  # VGA header-only library
│   │   ├── timer.c / timer.h      # Cycle-counter timer helpers
│   │   ├── printf.c / printf.h    # Minimal printf
│   │   └── softint64.c            # 64-bit integer helpers
│   ├── hello.c                    # UART Hello World
│   ├── led_test.c                 # LED blink
│   ├── gpio_test.c                # GPIO test
│   ├── vga_test.c                 # VGA color bars
│   ├── pong.c                     # UART Pong game
│   ├── space.c                    # VGA space shooter
│   ├── multiplication.c           # RV32IM multiply/divide test
│   ├── sdram_test.c               # SDRAM read/write test (BRAM execution)
│   ├── start.S                    # RISC-V startup code (BRAM programs)
│   ├── linker.ld                  # Linker script (BRAM, origin 0x0000)
│   ├── linker_app.ld              # Linker script (BRAM app, origin 0x1000)
│   ├── Makefile                   # GNU Make build system
│   ├── upload.py                  # UART bootloader client
│   └── elf2hex.py                 # ELF-to-HEX/MIF utility
│
├── doc/                            # Documentation
│   ├── SOFTWARE.md                # Complete build & run guide
│   ├── FPGA_DEPLOYMENT.md         # FPGA synthesis guide
│   ├── GPIO.md                    # LED/switch interfacing
│   ├── UART.md                    # Serial communication
│   ├── VGA.md                     # VGA controller and API
│   └── TIMER.md                   # 64-bit Timer and API
│
├── Z-Core.qsf                      # Quartus Pin Assignments & source files
├── Z-Core.sdc                      # Timing Constraints
└── LICENSE                         # MIT License
```

---

## Hardware Interface

### Clock and Reset

| Signal | Pin | Description |
|--------|-----|-------------|
| `MAX10_CLK1_50` | PIN_P11 | 50 MHz clock input |
| `KEY[0]` | PIN_B8 | Active-low reset |

### Debug Indicators

| LED | Function |
|-----|----------|
| `LEDR[0]` | System active (KEY[0] high) |
| `LEDR[2]` | AXI read address valid (instruction fetch) |
| `LEDR[3]` | AXI read address ready |
| `LEDR[7]` | GPIO[0] mirror |
| `LEDR[8]` | KEY[1] mirror |
| `LEDR[9]` | Heartbeat (~0.74 Hz) |

---

## Documentation

| Document | Description |
|----------|-------------|
| [SOFTWARE.md](doc/SOFTWARE.md) | Complete build, upload, and DOOM run guide |
| [FPGA_DEPLOYMENT.md](doc/FPGA_DEPLOYMENT.md) | FPGA synthesis and deployment |
| [GPIO.md](doc/GPIO.md) | LED and switch interfacing |
| [UART.md](doc/UART.md) | Serial communication |
| [VGA.md](doc/VGA.md) | VGA controller and API |
| [TIMER.md](doc/TIMER.md) | 64-bit Timer and API |

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

## Acknowledgements

- **[Alex Forencich](https://github.com/alexforencich)** — AXI-Lite interconnect and memory arbiter components from the [verilog-axi](https://github.com/alexforencich/verilog-axi) library.
- **SDRAM Controller** — IS42S16320D controller core used in the SDRAM subsystem.
- **DOOM RISC-V port** — originally by smunaut, adapted for Z-Core. ([Z-Core-DOOM](https://github.com/paudiaz99/Z-CORE-DOOM))

---

## References

- [Z-Core Processor Repository](https://github.com/paudiaz99/Z-Core)
- [DE10-Lite User Manual](https://faculty-web.msoe.edu/johnsontimoj/Common/FILES/DE10_Lite_User_Manual.pdf)
