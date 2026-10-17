# Whetstone (integer variant) — Z-Core (RV32IM bare-metal)

Classic mixed-workload benchmark adapted for integer-only RV32IM using Q16.16
fixed-point arithmetic. Exercises trig, exp/log/sqrt, array ops, branching,
and procedure calls. Reports **KWIPS** (Kilo Whetstone Integer Instructions
Per Second).

**License:** this port is MIT (see `whetstone_zcore.c`). Painter/netlib original
with required comment block: `software/linux/userspace/benchmarks/original/whetstone.c`.
See [THIRD_PARTY.md](../../../THIRD_PARTY.md).

## Build

```bash
make -f Makefile.zcore                    # default: 1000 loops
make -f Makefile.zcore WHET_LOOPS=5000    # custom count
```

## Upload & Run

```bash
cd ../../
python3 ../scripts/upload.py /dev/ttyUSB0 benchmarks/whetstone/whetstone_zcore.bin \
    --base 0x10000000 --entry 0x10000000
```

## Expected UART Output

```
=============================================================
 Whetstone (integer) -- Z-Core RV32IM @ 50 MHz
=============================================================
 Loops: 1000   (Q16.16 fixed-point, no FPU)
 Running 8 modules...

-------------------------------------------------------------
 Results
-------------------------------------------------------------
 Loops              : 1000
 Elapsed cycles     : <cycles>
 Elapsed time       : <ms> ms
 KWIPS              : <value>
=============================================================
```
