# Linux build and applications

The bundled Linux 6.12.10 Image embeds the complete root filesystem. The
Buildroot 2025.02.1 external tree is `software/linux/`. Kernel drivers, DT,
entry shim, configs, application packages and vendored application sources
are included. SHA256-pinned upstream archives are fetched into an ignored
`sources/` directory; no full kernel or compiler source dump is stored in Git.

## Boot after building

```sh
make fpga
make linux
make boot
make console
```

Log in as root with no password. Images default to `software/linux/build/images/`;
the bitstream defaults to `output_files/Z-Core.sof`. To inspect the plan without
hardware access:

```sh
./scripts/reboot.py --dry-run
```

The ELF/Image validator checks load segments and reserved memory bounds before
reset. Reload logs include exact image snapshots. Always load a matching
bitstream and kernel together.

## Rebuild from source

On Debian/Ubuntu, install build-essential, git, wget, curl, cpio, rsync, file,
bc, bison, flex, libssl-dev, libncurses-dev, python3, unzip, device-tree-compiler
and psmisc. Quartus is required for FPGA programming/building, not Linux builds.
Icarus Verilog and Verilator are needed only for simulation.

```sh
make linux JOBS=4
```

This fetches/verifies pinned Linux and Buildroot, configures RV32IMA/ILP32
without MMU/FPU/compressed instructions, builds the uClibc/bFLT toolchain and
all applications, then collects the kernel, ELF, DTB and entry shim in
`software/linux/build/images/`. The first toolchain build takes a long time.

Boot the newly built Linux files explicitly:

```sh
./scripts/reboot.py --images software/linux/build/images
```

To rebuild edited applications and the embedded rootfs incrementally:

```sh
make -C software/linux/build/buildroot zcore-apps-rebuild
make -C software/linux userspace images
```

The board driver hook patches only Linux 6.12.10 and rejects unexpected source
anchors. After driver edits, run `software/linux/scripts/prepare-kernel.py`
on `software/linux/build/buildroot/build/linux-6.12.10`, then invoke
`make -C software/linux/build/buildroot linux-rebuild` and rebuild userspace/images.

Applications use PIC bFLT with code/data loaded into RAM and 64 KiB stacks.
Vim disables linker relaxation to prevent absolute calls into low ROM addresses.
The no-MMU kernel needs contiguous physical allocations; DOOM requests a 6 MiB
zone, falls back to 3 MiB and reports allocation failure cleanly. WAD metadata
and cache tables are owned by that checked zone.

## Commands and limitations

The bundled application image uses the corrected FPGA bitstream. All
programs are static RV32IMA/ILP32 bFLT executables with 64 KiB stacks.
They use `-elf2flt=-r` to load PIC text and data together in RAM; the rootfs
hook rejects custom executables missing this flag.

## CPU and counters

`lscpu` is util-linux 2.40.2. `/proc/cpuinfo` reports the Z-Core vendor/model
names and the actual numeric `mvendorid`, `marchid`, and `mimpid` separately.
Numeric vendor ID 0 remains unassigned; no manufacturer ID is invented.

`perf` is a small **Z-Core counting frontend**, not the full upstream perf
distribution. It uses Linux `perf_event_open`, enable/disable ioctls, and reads;
the kernel supplies normal event accounting. Supported commands are `list` and
`stat`. Hardware has no overflow interrupt, so sampling and `record` are unavailable. `-u` selects hardware User-mode counters;
kernel-only counting is unavailable.

```sh
lscpu
perf list
zcore-counters
perf stat -e cycles,instructions,branch-misses -- fpga-dhrystone-quick
perf stat -a -e cycles,instructions,icache-hits,icache-misses --timeout 1000
```

Events are enabled and disabled separately, so their intervals differ by
system-call overhead. All are stopped before UART reporting.
The task measurement includes the frontend's launch/wait overhead. `-a` counts
the whole CPU, including idle, kernel activity, and other processes. Reads in
`zcore-counters` are system-wide snapshots since reset, not an atomic snapshot
of all counters at once. Each 64-bit read uses high/low/high retry.

| Raw event/CSR index | Event |
|---|---|
| 0 | cycles |
| 2 | instructions |
| 3 | icache-hits |
| 4 | dcache-hits |
| 5 | load-requests |
| 6 | store-requests |
| 7 | branch-misses |
| 8 | pipeline-flushes |
| 9 | icache-misses |
| 10 | memory-fetches |
| 11 | uncached-accesses |
| 12 | dcache-misses |

`icache-hits` and `icache-misses` count retired instructions, including FENCE.I.
Each retired instruction increments one of them: a hit when it was consumed
from the instruction cache, a miss when it was consumed from a refill. Their
sum equals `instructions`.

`dcache-misses` counts each cached demand load/store whose tag lookup misses
once, including write-buffer forwarding and stalled requests. Warm hits,
uncached MMIO, refill writes and cache maintenance do not increment it.

```sh
perf stat -e cycles,instructions,dcache-misses -u coremark
perf stat -e dcache-misses -- ptr-chase-quick
```

`-u` (or `--user`) excludes Machine-mode kernel execution using a separate
hardware counter bank. The standard counters continue to report total activity.
Retired instructions retain their originating privilege through the pipeline;
User-mode loads/stores retain it in MEM. The frontend's own User-mode launch/wait
work is still included. A kernel and bitstream with the new counter bank are
required together; the prior eleven-counter bitstream cannot run this kernel.

Event semantics follow the RTL's existing pulse inputs. `cpu/event=3/` and
`r3` also select raw event 3. Userspace read-only CSR aliases are enabled; no
userspace counter writes or privileged CSR access are enabled.

## Benchmarks

The main commands use original Linux/Unix sources wherever available:

| Command | Source and behavior |
|---|---|
| `coremark` | EEMBC CoreMark algorithms with its Linux port, calibrated runtime and CRC validation |
| `dhrystone` | Netlib Dhrystone 2.1, separate compilation, libc string routines and Unix process CPU timing |
| `whetstone` | Netlib C double-precision Whetstone 1.2; uses software floating point on RV32IMA |
| `stream` | McCalpin STREAM 5.10, original double-precision kernels and numeric validation |
| `lmbench-*` | Original memory/syscall kernels with an explicitly labelled sequential no-MMU harness |
| `ptr-chase`, `wb3d` | Linux ports of the FPGA-specific memory/write-buffer stress tests |

Upstream copies and exact download hashes are in `userspace/benchmarks/original/SOURCES.json`.
Sources: [Dhrystone](https://www.netlib.org/benchmark/dhry-c),
[Whetstone](https://www.netlib.org/benchmark/whetstone.c),
[STREAM](https://www.cs.virginia.edu/stream/FTP/Code/stream.c).
The Dhrystone build updates old libc declarations for modern headers without
changing its timed loops. It uses `times()` and Linux's 100 Hz user accounting.

```sh
coremark
printf '1000000\n' | dhrystone
whetstone 100
stream
lmbench-bw_mem -N 1 64k rd
lmbench-lat_mem_rd -N 1 1 64
lmbench-lat_syscall -N 1 null
ptr-chase
wb3d
```

CoreMark must pass CRC validation and its 10-second minimum runtime for a valid
score. Dhrystone prompts for the iteration count; increase it if its duration
warning appears. Whetstone accepts a loop count and defaults to 1000; software
floating point can make this slow. Follow each original benchmark's reporting
rules and retain compiler settings with results.

STREAM uses its supported compile-time array-size setting: 131072 doubles per
array (1 MiB each), three arrays, 10 repetitions. Each array exceeds 20 times
this CPU's 32 KiB D-cache. `stream-quick` uses 16384 doubles per array for
bring-up only, below that recommended footprint, so do not publish its output
as a standard STREAM result.

The earlier FPGA adaptations remain under `fpga-dhrystone`, `fpga-whetstone`,
and `fpga-stream`, plus their `-quick` versions. These are comparison variants;
FPGA Whetstone is fixed-point and FPGA STREAM is int32. The main commands above
use the original benchmark arithmetic instead.

The FPGA stress-test ports use Linux-owned memory and libc I/O and return to
the shell. WB3D retains the original relative scratch layout, uses libc memory
routines, and checks all six phases without the endless VGA animation. Full
pointer chase retains its 8 MiB sweep; `ptr-chase-quick` allocates just 64 KiB.
The kernel permits contiguous allocations up to 32 MiB for large bFLT programs
and application heaps. Linux interrupts and scheduling contribute to elapsed cycle
measurements; these are Linux runs at the current 50 MHz board clock.

lmbench's fork-based original harness cannot run on this no-MMU libc. The
installed memory/syscall tests retain their kernels but use a single-worker
harness; only `-P 1` is supported. Treat their reports as no-MMU variants, not
an unmodified full lmbench suite. Multiprocess/network tests are unavailable.

## Display and terminal programs

```sh
doom
doom --frames 2
doom -warp 1 1 -skill 3
top
htop
nano /tmp/example.txt
vim /tmp/example.txt
```

DOOM reads `/usr/share/doom/doom1.wad` through Linux. Its display backend writes
320x200 RGB332 frames to `/dev/zcore-vga`; the driver sends packed pixels to
the FPGA VGA registers. VGA remains the display output. WASD moves/turns,
space fires, E uses, Enter selects, Escape opens the menu, and Ctrl-C quits.
The Linux zone allocator requests 6 MiB, with a 3 MiB fallback, and exits
cleanly if allocation fails. This avoids requiring a contiguous 16 MiB physical
block for the previous 12 MiB request. WAD directory/cache tables also use the checked zone;
appending a WAD preserves existing entries and override order. The repair is
installed and verified on the board, including a start with no 16 MiB block
available.

Controls are read from the interactive UART terminal while the picture stays
on VGA. UART supplies characters rather than key-release events, so each key
automatically releases after four game tics (about 114 ms); keyboard repeats
extend it. The current input backend keeps one control key active at a time.
Number keys select weapons and Tab toggles the map. Sound and networking remain disabled. Eight-byte WAD-name lookups reserve a
terminator so uppercase conversion cannot overwrite the stack. `--frames N` exits after N frame writes. `-warp 1 MAP` starts a shareware
map directly; `-skill 1..5` chooses difficulty.

Nano 8.2 uses its tiny build. Vim 9.1.0145 uses its tiny build, with no runtime
syntax files; a minimal defaults.vim supplies basic startup settings. Its link disables relaxation to prevent absolute calls near
address zero in relocated bFLT code. htop 3.3.0 is static; optional fork-based external helpers
(lsof, strace, systemctl) are unavailable. Normal process viewing works.
Terminal programs use ANSI escape sequences, so watch them through the
interactive console, not through a tailed log. UART throughput and the
one-byte RX register constrain refresh rate and input. Transmit bursts send
up to 16 bytes within a 1.5 ms budget, with the 5 ms polling interval retained.

```sh
python3 scripts/console.py --interactive
```

The login profile sets `TERM=xterm-256color` and an 80x24 screen. Change it
with `stty rows N cols N` if needed. Ctrl-] disconnects without stopping Linux.


## RTL simulation

```sh
make test
make test-rtl
python3 scripts/sim-linux.py --commands /path/to/commands.txt
```

The command file starts with `root`, then Linux shell commands, one per line.
The model runs the actual bundled Image on the shipped CPU/cache RTL with ideal
AXI RAM and paced UART frames. It does not model SDRAM, CDC or physical timing.
