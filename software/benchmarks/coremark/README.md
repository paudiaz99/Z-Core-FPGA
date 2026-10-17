# CoreMark on Z-Core

The original EEMBC kernels are in `coremark/`; the RV32IM bare-metal port is
in `z_core/`. No submodule is needed. Component licenses remain with the sources.

From the repository root:

```sh
make -C software coremark
python3 scripts/upload.py /dev/ttyUSB0 software/benchmarks/coremark/coremark.bin --reset --base 0x10000000 --entry 0x10000000
```

A `riscv32-unknown-elf-` toolchain is required. The default is 2000 iterations.
Scores require matching CRCs and a run of at least ten seconds. The separate
Linux build uses the POSIX port in `software/linux/userspace/benchmarks/coremark/`.
