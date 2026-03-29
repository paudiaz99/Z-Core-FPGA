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
![RISC-V](https://img.shields.io/badge/RISC--V-RV32IM-green?logo=riscv)
[![FPGA](https://img.shields.io/badge/FPGA-MAX%2010-0071c5.svg)](https://www.altera.com/asap/offering/a1jui0000049upbmam/max-10-device-family-de10-lite-board)
[![Board](https://img.shields.io/badge/Board-DE10--Lite-00a98f.svg)](https://www.terasic.com.tw/cgi-bin/page/archive.pl?Language=English&No=1021)


</div>


---

## Block Diagram

<div align="center">
  <img src="https://github.com/user-attachments/assets/fec3a0e0-5cef-46e4-a3fd-7f07b1387a11" alt="centered image">
  <br>
  <sup>Z-Core SoC Architecture.</sup>
</div>

## Z-Core RV32IM Architecture
<div align="center">
  <img src="https://github.com/user-attachments/assets/c02b2a54-ae7c-4070-adcd-875faa8720d2" alt="centered image">
  <br>
  <sup>Z-Core RV32IM Architecture Diagram.</sup>
</div>


> **Note**: For Z-Core detailed processor architecture explanation, pipeline implementation, verification methodology, and ISA compliance documentation, refer to the main **[Z-Core repository](https://github.com/paudiaz99/Z-Core)**.

---

## Overview

This repository contains the FPGA implementation of the Z-Core RISC-V RV32IMZicsr processor, targeting the Intel DE10-Lite development board. It includes a VGA Controller to display a toy example (vga_test.c), space invaders game (space.c),
or whatever you can think of :D.

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
| ISA        | RV32IM + Zicsr |
| Features   | Instruction Cache, Branch Predictor |
| Peripherals | UART, GPIO, VGA (160x120), 64-bit Timer |
| Development Board | Terasic DE10-Lite |

---

## Prerequisites

| Tool | Purpose |
|------|---------|
| Intel Quartus Prime Lite | FPGA synthesis and programming |
| RISC-V GNU Toolchain | Cross-compilation (rv32im target) |
| Python 3.x | ELF-to-HEX conversion |

---

## Quick Start

### 1. Compile Software

```bash
cd software
./hex_gen.sh -Target hello
```

### 2. Synthesize FPGA Design

1. Create new Quartus Prime project for MAX 10 (10M50DAF484C7G)
2. Add RTL files from `rtl/` directory
3. Import settings: `Assignments → Import Assignments` → select `Z-Core.qsf`
4. Add timing constraints: `Z-Core.sdc`
5. Run Analysis & Synthesis (`Ctrl+K`)
6. Run Fitter (`Ctrl+L`)

### 3. Program FPGA

1. Open Programmer (`Tools → Programmer`)
2. Load `output_files/Z-Core.sof`
3. Click Start

### 4. Verify Operation

| Indicator | Expected Behavior |
|-----------|-------------------|
| `LEDR[9]` | Heartbeat blink (~0.74 Hz) |
| `LEDR[7:0]` | GPIO output register |
| `KEY[0]` | Press to reset CPU |

---

## Software

The software is located in the `software/` directory. It contains multiple examples that can be compiled and run on the Z-Core processor. These are compiled using the RISC-V GNU Toolchain and the Makefile provided in the directory.

### Examples

- `hello`: A simple "Hello, World!" program. Sends messages via UART.
- `led_test`: A program that blinks the 10 LEDs on the DE10-Lite board.
- `gpio_test`: A program that tests the GPIO functionality.
- `pong`: Classic Pong game rendered via UART.
- `vga_test`: Displays color bars and a bouncing square via VGA.
- `space`: "Star Assault" space shooter game for VGA.
- `multiplication`: Test suite for the RV32IM multiplication/division instructions.

### Pong Game Setup

The Pong game is a simple implementation of the classic Pong game. It uses the UART to display the game and the GPIO to control the paddles. Currently the only controllable paddle is Player 1 (the one on the left). Feel free to add Player 2 control via the GPIO.

<div align="center">
  <img src="https://github.com/user-attachments/assets/b50e7727-6a6f-46b3-bfb0-b8266959b0bc" alt="centered image">
  <br>
  <sup>Pong Game via UART.</sup>
</div>

The Game will also display the score on the LEDs connected to GPIOs [7:0].

**Input GPIO Connection**

| GPIO | Function |
|------|----------|
| 8    | Paddle 1 Up |
| 9    | Paddle 1 Down |

>**Note** : You will need two push buttons and two pull down resistors to control the paddles. Connect the buttons to the GPIO and the pull down resistors to GND.

---

## Memory Map

| Address Range | Peripheral | Size |
|---------------|------------|------|
| `0x0000_0000` - `0x0000_3FFF` | Block RAM (Boot + App) | 16 KB |
| `0x0400_0000` - `0x0400_0FFF` | UART | 4 KB |
| `0x0400_1000` - `0x0400_1FFF` | GPIO | 4 KB |
| `0x0400_2000` - `0x0400_2FFF` | Timer | 4 KB |
| `0x0400_3000` - `0x0400_3FFF` | VGA | 4 KB |

> [!IMPORTANT]
> **Memory Limitation**: The system uses **4 KB** of on-chip Block RAM for program memory, not the external SDRAM (64 MB). Programs must fit within this limit. Increase `ADDR_WIDTH` in `axil_ram` instantiation for larger memory.
The capability of using the 64MB SDRAM will be added in the future.

---

## Project Structure

```
├── rtl/                        # Synthesizable RTL
│   ├── z_core_top_model.v     # Top-level FPGA wrapper
│   ├── z_core_control_u.v     # CPU Control Unit
│   ├── z_core_alu.v           # Arithmetic Logic Unit
│   ├── z_core_alu_ctrl.v      # ALU Control Unit
│   ├── z_core_decoder.v       # Instruction Decoder
│   ├── z_core_reg_file.v      # General Purpose Registers
│   ├── z_core_csr_file.v      # CSR File (Zicsr)
│   ├── z_core_instr_cache.v   # Instruction Cache
│   ├── z_core_branch_pred.v   # Branch Predictor
│   ├── z_core_mult_unit.v     # Multiplier Unit
│   ├── z_core_div_unit.v      # Division Unit
│   ├── axil_interconnect.v    # AXI-Lite Bus Interconnect
│   ├── axil_timer.v           # 64-bit Timer Peripheral
│   ├── axil_vga.v             # VGA Controller Peripheral
│   ├── axil_uart.v            # UART Peripheral
│   ├── axil_gpio.v            # GPIO Peripheral
│   ├── axil_master.v          # AXI-Lite Master Interface
│   ├── axi_mem.v              # AXI-Lite RAM Interface
│   └── flist.vc               # File list for synthesis
│
├── software/                   # Example programs and tools
│   ├── bootloader/            # M9K Initial Bootloader
│   │    ├── bootloader.c          # Bootloader source
│   │    ├── Makefile              # Bootloader build system
│   │    └── linker_boot.ld        # Bootloader-specific linker
│   ├── libs/                  # Libraries
│   │    ├── uart.c                # UART Library
│   │    ├── uart.h                # UART header
│   │    └── vga.h                 # VGA header-only library
│   ├── hello.c                # UART Hello World
│   ├── led_test.c             # LED blink example
│   ├── gpio_test.c            # GPIO logic test
│   ├── vga_test.c             # VGA color bars & animation
│   ├── pong.c                 # UART Pong game
│   ├── space.c                # VGA Space shooter
│   ├── multiplication.c       # RV32IM instruction test
│   ├── start.S                # RISC-V Startup code
│   ├── linker.ld              # Main linker script
│   ├── linker_app.ld          # Application linker (origin 0x1000)
│   ├── Makefile               # GNU Make build system
│   ├── upload.py              # UART bootloader client
│   ├── hex_gen.sh             # Build helper script
│   └── elf2hex.py             # HEX/MIF generation utility
│
├── doc/                        # Documentation
│   ├── FPGA_DEPLOYMENT.md     # Complete deployment guide
│   ├── GPIO.md                # LED/Switch interfacing
│   ├── UART.md                # Serial communication
│   ├── VGA.md                 # VGA controller and API
│   └── TIMER.md               # 64-bit Timer and API
│
├── Z-Core.qsf                  # Quartus Pin Assignments
├── Z-Core.sdc                  # Timing Constraints
└── LICENSE                     # MIT License
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
| `LEDR[7:0]` | GPIO output register |
| `LEDR[8]` | AXI-Lite read activity |
| `LEDR[9]` | System heartbeat |

---

## Documentation

| Document | Description |
|----------|-------------|
| [FPGA_DEPLOYMENT.md](doc/FPGA_DEPLOYMENT.md) | Complete deployment guide |
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

---

## References

- [Z-Core Processor Repository](https://github.com/paudiaz99/Z-Core)
- [DE10-Lite User Manual](https://ftp.intel.com/Public/Pub/fpgaup/pub/Intel_Material/Boards/DE10-Lite/DE10_Lite_User_Manual.pdf)
