# Dhrystone 2.1 — Z-Core (RV32IM bare-metal)

Classic integer performance benchmark by Reinhold P. Weicker (1984/1988).
Reports **Dhrystones/sec**, **DMIPS**, and **DMIPS/MHz**.

**License:** Z-Core port harness is MIT (see `dhrystone_zcore.c`). Upstream
netlib sources and rationale (no OSI SPDX on the historical tree):
`software/linux/userspace/benchmarks/original/`. See [THIRD_PARTY.md](../../../THIRD_PARTY.md).

## Build

```bash
make -f Makefile.zcore                            # default: 500000 iterations
make -f Makefile.zcore DHRY_ITERATIONS=1000000    # custom count
```

## Upload & Run

```bash
cd ../../
python3 ../scripts/upload.py /dev/ttyUSB0 benchmarks/dhrystone/dhrystone_zcore.bin \
    --base 0x10000000 --entry 0x10000000
```

## Expected UART Output

```
=============================================================
 Dhrystone 2.1 -- Z-Core RV32IM @ 50 MHz
=============================================================
 Iterations: 500000
 Running...

-------------------------------------------------------------
 Results
-------------------------------------------------------------
 Iterations         : 500000
 Elapsed cycles     : <cycles>
 Elapsed time       : <ms> ms
 Dhrystones/sec     : <value>
 DMIPS              : <value>
 DMIPS/MHz          : <value>
=============================================================
```
