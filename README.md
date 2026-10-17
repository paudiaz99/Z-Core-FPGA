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
![RISC-V](https://img.shields.io/badge/RISC--V-RV32IMA-green?logo=riscv)
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

## Z-Core Architecture
<div align="center">
  <img src="https://github.com/user-attachments/assets/5d1e0237-2ab6-4d9c-bd10-bd1ab6458d75" alt="centered image">
  <br>
  <sup>Z-Core processor architecture.</sup>
</div>


> **Note**: For Z-Core detailed processor architecture explanation, pipeline implementation, verification methodology, and ISA compliance documentation, refer to the main **[Z-Core repository](https://github.com/paudiaz99/Z-Core)**, or the following website, where I dive into the details of the processor: https://paudiazcuesta.com/work/z-core/.

---

## Overview

This repository contains the FPGA implementation of the Z-Core RISC-V processor, targeting the Intel DE10-Lite development board (MAX 10 FPGA). The current core implements RV32IMA, Zicsr and Zifencei, with a 16 KB direct-mapped instruction cache, a 32 KB 2-way set-associative write-back/write-allocate data cache, a hybrid branch predictor and a 64 MB SDRAM interface.

The system supports bare-metal programs and no-MMU Linux. Small programs run from on-chip Block RAM (16 KB); larger programs and Linux are loaded into SDRAM through the UART bootloader. Linux runs the kernel in Machine mode and userspace in User mode, with VGA DOOM, benchmarks and terminal tools.

The Quartus project, SDRAM IP, software and matched Linux boot files are included here. No sibling checkout, external drive or submodule is required. See the [main Z-Core repository](https://github.com/paudiaz99/Z-Core) for processor development.

---

<div align="center">
  <video src="https://github.com/user-attachments/assets/7ed1b9ca-17da-4339-a832-d4b44bc0be02" width="100%" autoplay loop muted playsinline></video>
  <br>
  <sup>Z-Core Running Doom @ 10 FPS (320x200).</sup>
</div>

---

### Key Specifications

| Parameter | Value |
|-----------|-------|
| Target FPGA | Intel MAX 10 (10M50DAF484C7G) |
| Operating Frequency | 50 MHz |
| ISA | RV32IMA + Zicsr + Zifencei |
| Privilege | Machine/User modes; no MMU |
| Instruction Cache | 16 KB direct-mapped (4096 entries) |
| Data Cache | 32 KB 2-way set-associative (write-back, write-allocate) |
| Branch Predictor | 2-Bit Branch Predictor |
| BRAM | 16 KB on-chip (bootloader + small apps) |
| SDRAM | 64 MB IS42S16320D @ 100 MHz (via AXI-CDC bridge) |
| Peripherals | UART, GPIO (16-bit), VGA (320×200 RGB332), 64-bit Timer |
| Performance Counters | Twelve fixed 64-bit counters, including D-cache misses and User-mode filtering |
| Development Board | Terasic DE10-Lite |

---

## Performance

All measurements on DE10-Lite (MAX 10) at 50 MHz running bare-metal. For a detailed performance analysis on Z-Core, visit: https://paudiazcuesta.com/posts/z-core-performance/.

### CoreMark

| Metric | Value |
|--------|-------|
| CoreMark/MHz | **3.24** |

### DOOM (300×200, `doom1.wad` shareware)

| Configuration | FPS |
|---------------|-----|
| No cache (SDRAM only) | < 1 FPS |
| 32 KB Instruction Cache only | ~3 FPS |
| 32 KB I-Cache + 32 KB 2-way D-Cache | **~11 FPS** |
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
| Python 3.x | ELF-to-HEX conversion and UART upload scripts |
| psmisc (`fuser`) | Check UART ownership before reset/upload |
| Icarus Verilog / Verilator | Optional RTL checks / Linux boot simulation |

Rebuilding Linux also needs the host packages listed in [doc/LINUX.md](doc/LINUX.md); Buildroot builds the no-MMU cross-toolchain.

---

## Quick Start (UART Bootloader)

The Z-Core SoC includes a UART bootloader stored in Block RAM, allowing you to upload and run software without recompiling the FPGA hardware.

### Linux

Connect the powered DE10-Lite, USB-Blaster and a 3.3 V UART adapter with common ground. Close other UART terminals before resetting/uploading. Put Quartus `bin` on PATH, or pass `--quartus-pgm /path/to/quartus_pgm` to the reload script.

```sh
make boot
make console
```

Build first (`make fpga` and `make linux`), then `make boot` programs `output_files/Z-Core.sof`, resets through UART BREAK, uploads `software/linux/build/images/` (kernel/shim/DTB) at 115200 baud and waits for login. A typical Image takes on the order of ten minutes to upload. Log in as `root` with no password. Ctrl-] exits the console and leaves Linux running.

To reload Linux when the matching bitstream is already installed:

```sh
./scripts/reboot.py
./scripts/reboot.py --dry-run
```

Reloads save exact image copies and logs under `logs/reboot/` and print a `tail -f` command. The twelve-counter kernel and bitstream must be loaded together.

### Small programs (BRAM, ≤ 12 KB)

```sh
make -C software/tests APP=1 hello.bin
python3 scripts/upload.py /dev/ttyUSB0 software/tests/hello.bin --reset
```

### DOOM on Linux (VGA, `doom1.wad` shareware)

The bundled Linux image includes DOOM. Run from the Linux shell:

```sh
doom > /tmp/doom.log 2>&1
```

VGA displays the picture; UART supplies controls. Esc opens the menu, WASD moves/turns, Space fires, E uses, Enter selects, 1–7 selects weapons, Tab opens the map and Ctrl-C quits. `doom -warp 1 1` enters gameplay directly. Sound and network play are disabled. The earlier [bare-metal DOOM port](https://github.com/paudiaz99/Z-CORE-DOOM) remains a separate project.

### Linux tools and benchmarks

The image also includes lscpu, top, htop, nano, Vim tiny, perf counting, CoreMark, original Dhrystone/Whetstone/STREAM, pointer chase, WB3D and three lmbench kernels.

```sh
lscpu
perf list
perf stat -e cycles,instructions,dcache-misses -u coremark
nano /tmp/example.txt
htop
```

Files in the initramfs are lost on reset or power-off. Storing the FPGA configuration in flash does not store the Linux Image or provide persistent folders.

### Quick Reference

| What | Command |
|------|---------|
| Build FPGA | `make fpga` |
| Rebuild Linux | `make linux` |
| Program FPGA + upload Linux | `make boot` |
| Linux console | `make console` |
| Build BRAM example | `make -C software/tests APP=1 hello.bin` |
| Upload bare-metal program | `python3 scripts/upload.py /dev/ttyUSB0 software/tests/hello.bin --reset` |
| Build bare-metal benchmarks | `make benchmarks` |
| Check images and project | `make test` |
| RTL regressions / Linux simulation | `make test-rtl` / `make sim-linux` |

See [doc/LINUX.md](doc/LINUX.md) for Linux builds, programs and controls, and [doc/SOFTWARE.md](doc/SOFTWARE.md) for bare-metal software.

---

## Software Examples

The `software/tests/` directory contains examples compiled with the RISC-V GNU Toolchain. Shared drivers live in `software/libs/`; larger benchmark programs are in `software/benchmarks/`.

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
| `linux/` | Linux board port, applications, benchmarks and VGA DOOM |

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

### Historical Bare-Metal DOOM SDRAM Layout

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

```text
├── rtl/
│   ├── z_core_top_model.v
│   ├── z_core_control_u.v
│   ├── z_core_csr_file.v
│   ├── z_core_instr_cache.v
│   ├── z_core_data_cache.v
│   ├── z_core_write_buffer.v
│   ├── axil_*.v
│   ├── axi_sdram_bridge.v
│   ├── sdram_axi_top.v
│   ├── sdram_pll0.v / .qip
│   └── Sdram_Control/
├── software/
│   ├── bootloader/
│   ├── libs/
│   ├── tests/
│   ├── benchmarks/
│   ├── sdram_test/
│   └── linux/
├── scripts/
├── doc/
├── Z-Core.qpf
├── Z-Core.qsf
├── Z-Core.sdc
├── Makefile
└── LICENSE
```

`rtl/` contains the processor, peripherals and local SDRAM IP. Linux images land in `software/linux/build/images/` after `make linux`; the bitstream in `output_files/` after `make fpga`. Build outputs, source downloads, Quartus databases, logs and any local `prebuilt/` cache are gitignored.

The Linux Image loads at `0x10000000`, its entry shim at `0x13fe0000`, and its DTB at `0x13ff0000`. The final 128 KiB of SDRAM is reserved for the shim and DTB.

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
| [SOFTWARE.md](doc/SOFTWARE.md) | Bare-metal builds, examples and upload guide |
| [HARDWARE.md](doc/HARDWARE.md) | Quartus project, wiring, peripherals and memory map |
| [CPU.md](doc/CPU.md) | Pipeline, privilege, caches and performance counters |
| [LINUX.md](doc/LINUX.md) | Linux build, boot, applications and limitations |

**Timing at 50 MHz is not closed.** The bundled counter release passed Quartus, directed RTL tests and Linux simulation; it has not been uploaded to the board.

---

## License

**Original** project RTL and board software are under the MIT License — see [LICENSE](LICENSE). That MIT grant does **not** cover the whole tree.

Third-party and vendor components (CoreMark, STREAM, lmbench, DOOM engine, Forencich AXI, Painter Whetstone originals, Dhrystone provenance, Altera Quartus IP, Terasic-style SDRAM control, etc.) keep their own terms. Full inventory, restrictions (including lmbench result publication rules), and non-MIT FPGA IP notes: **[THIRD_PARTY.md](THIRD_PARTY.md)**. Linux packaging GPLv2 text: `software/linux/LICENSE`.

---

## Acknowledgements

- **[Alex Forencich](https://github.com/alexforencich)** — AXI-Lite interconnect and memory arbiter components from the [verilog-axi](https://github.com/alexforencich/verilog-axi) library.
- **SDRAM Controller** — Terasic-style IS42S16320D controller under `rtl/Sdram_Control/` (not MIT; see [THIRD_PARTY.md](THIRD_PARTY.md)); Altera-generated PLL/FIFO IP likewise.
- **DOOM RISC-V port** — originally by smunaut, adapted for Z-Core (`software/linux/userspace/doom/`, GPLv2). IWAD not shipped.
- **[lavat](https://github.com/AngelJumbo/lavat)** — by AngelJumbo; not vendored here (source builds omit it).

---

## References

- [Z-Core Processor Repository](https://github.com/paudiaz99/Z-Core)
- [DE10-Lite User Manual](https://faculty-web.msoe.edu/johnsontimoj/Common/FILES/DE10_Lite_User_Manual.pdf)
