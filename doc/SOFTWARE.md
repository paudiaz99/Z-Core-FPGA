# Bare-metal software

Linux applications and commands are described in [LINUX.md](LINUX.md).
Bare-metal sources remain available for CPU/peripheral experiments.

Install a `riscv32-unknown-elf-` toolchain and put it on PATH. These programs
use RV32IM/ILP32, UART output and the shared runtime in `software/libs/`.

```sh
make bootloader
make examples
make -C software/tests APP=1 pong.bin
make benchmarks
```

The bootloader build regenerates its four BRAM MIF lanes. Example binaries
load at `0x1000`, above the 4 KiB ROM code, when built with `APP=1`:

```sh
make -C software/tests APP=1 hello.bin
python3 scripts/upload.py /dev/ttyUSB0 software/tests/hello.bin --reset --base 0x1000 --entry 0x1000
```

The binary base/entry must match its linker script. SDRAM benchmark binaries
load at `0x10000000`; resetting/uploading one replaces the running Linux image.

| Benchmark | Build command from repository root |
|---|---|
| Dhrystone | `make -C software/benchmarks/dhrystone -f Makefile.zcore` |
| Whetstone | `make -C software/benchmarks/whetstone -f Makefile.zcore` |
| Integer STREAM | `make -C software/benchmarks/STREAM -f Makefile.zcore` |
| Pointer chase | `make -C software/benchmarks/ptr_chase -f Makefile.zcore` |
| WB3D | `make -C software/benchmarks/wb3d -f Makefile.zcore` |
| CoreMark | `make -C software coremark` |

For example:

```sh
python3 scripts/upload.py /dev/ttyUSB0 software/benchmarks/dhrystone/dhrystone_zcore.bin --reset --base 0x10000000 --entry 0x10000000
```

CoreMark uses its Z-Core port and original timed kernels; a valid score needs
at least ten seconds and matching CRCs. Its default bare-metal build uses 2000
iterations. WB3D disables GCC's loop-pattern conversion so its freestanding
memset/memcpy implementations cannot turn into recursive calls.

The Linux image includes original floating-point STREAM/Whetstone and original
Dhrystone as the main commands. FPGA adaptations are labelled `fpga-*` there.
lmbench's supported kernels and its no-MMU sequential harness live in
`software/linux/userspace/benchmarks/lmbench/`; the unused legacy framework and
old DOOM submodule are replaced by the Linux source port.
