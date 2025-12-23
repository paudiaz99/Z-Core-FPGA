<div align="center">
<pre>
███████╗       ██████╗ ██████╗ ██████╗ ███████╗
╚══███╔╝      ██╔════╝██╔═══██╗██╔══██╗██╔════╝
  ███╔╝ █████╗██║     ██║   ██║██████╔╝█████╗  
 ███╔╝  ╚════╝██║     ██║   ██║██╔══██╗██╔══╝  
███████╗      ╚██████╗╚██████╔╝██║  ██║███████╗
╚══════╝       ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝
</pre>
</div>

<div align="center">
  
**FPGA Implementation for DE10-Lite Board**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![FPGA](https://img.shields.io/badge/FPGA-MAX%2010-0071c5.svg)](https://www.altera.com/asap/offering/a1jui0000049upbmam/max-10-device-family-de10-lite-board)
[![Board](https://img.shields.io/badge/Board-DE10--Lite-00a98f.svg)](https://www.terasic.com.tw/cgi-bin/page/archive.pl?Language=English&No=1021)

</div>


---

> **Note**: For Z-Core processor architecture, pipeline implementation, verification methodology, and ISA compliance documentation, refer to the main **[Z-Core repository](https://github.com/paudiaz99/Z-Core)**.

---

## Overview

This repository contains the FPGA implementation of the Z-Core v0.2.0-alpha RISC-V RV32I processor, targeting the Intel DE10-Lite development board.

### Key Specifications

| Parameter | Value |
|-----------|-------|
| Target FPGA | Intel MAX 10 (10M50DAF484C7G) |
| Operating Frequency | 50 MHz |
| Program Memory | 4 KB Block RAM (M9K) |
| Peripherals | UART (9600 baud), GPIO |
| Development Board | Terasic DE10-Lite |

---

## Prerequisites

| Tool | Purpose |
|------|---------|
| Intel Quartus Prime Lite | FPGA synthesis and programming |
| RISC-V GNU Toolchain | Cross-compilation (rv32i target) |
| Python 3.x | ELF-to-HEX conversion |

---

## Quick Start

### 1. Compile Software

```powershell
cd software
./hex_gen.ps1 -Target hello
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

## Memory Map

| Address Range | Peripheral | Size |
|---------------|------------|------|
| `0x0000_0000` - `0x0000_0FFF` | Block RAM | 4 KB |
| `0x0400_0000` - `0x0400_0FFF` | UART | 4 KB |
| `0x0400_1000` - `0x0400_1FFF` | GPIO | 4 KB |

> [!IMPORTANT]
> **Memory Limitation**: The system uses **4 KB** of on-chip Block RAM (M9K) for program memory, not the external SDRAM (64 MB). Programs must fit within this limit. Increase `ADDR_WIDTH` in `axil_ram` instantiation for larger memory.
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
│   ├── axil_interconnect.v    # AXI-Lite Bus Interconnect
│   ├── axil_master.v          # AXI-Lite Master Interface
│   ├── priority_encoder.v     # Priority Encoder
│   ├── axi_mem.v              # AXI-Lite RAM (4KB Block RAM)
│   ├── axil_uart.v            # UART Peripheral
│   ├── axil_gpio.v            # GPIO Peripheral
│   └── arbiter.v              # Bus Arbiter logic
│
├── software/                   # Example programs and tools
│   ├── hello.c                # Main example application
│   ├── led_test.c             # LED peripheral test
│   ├── start.S                # RISC-V Startup code
│   ├── linker.ld              # Linker script
│   ├── Makefile               # GNU Make build system
│   └── elf2hex.py             # HEX generation utility
│
├── doc/                        # Documentation
│   ├── FPGA_DEPLOYMENT.md     # Detailed deployment guide
│   ├── GPIO.md                # LED/Switch interfacing
│   └── UART.md                # Serial communication
│
├── Z-Core.qsf                  # Quartus Pin Assignments
└── Z-Core.sdc                  # Timing Constraints
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
