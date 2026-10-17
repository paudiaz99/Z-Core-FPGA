# CPU architecture

The shipped core implements RV32IMA, Zicsr and Zifencei, with Machine and User
modes and no MMU. It has a five-stage pipeline, forwarding, load/store hazard
tracking, dynamic branch prediction, iterative division, instruction/data
caches and an eight-entry write buffer. Linux runs its kernel in M-mode and
bFLT processes in U-mode. This configuration does not provide address-space
isolation through an MMU.

The FPGA configuration has a 4096-entry direct-mapped instruction cache
(16 KiB) and a two-way, 4096-entry-per-way data cache (32 KiB), with four-byte
lines. SDRAM is cacheable; peripheral MMIO bypasses caches. Physical memory
attributes are defined in `rtl/z_core_pma_map.vh`.

## Linux-relevant behavior

- LR/SC and AMO instructions implement the A extension, with alignment traps,
  reservation invalidation and ordered memory updates.
- MRET restores M/U privilege from MPP; traps save the previous privilege.
  User ECALL reports cause 8; Machine ECALL reports cause 11.
- CSR accesses check privilege, implemented addresses and read-only writes.
  `mcounteren` controls User-mode access to read-only counter aliases.
- WFI is legal in M-mode and acts as a NOP; User-mode WFI traps.
- FENCE orders older memory work. FENCE.I drains the pipeline/write buffer,
  cleans dirty data-cache lines, invalidates the instruction cache and resumes
  at the following instruction. This is required by relocated/self-modifying
  executable loading.
- A stalled mispredicted branch permits older write-buffer entries to drain.
  Blocking the drain while its prediction flush was asserted caused a proven
  pipeline deadlock during Linux boot.

## CSRs and counters

Machine trap CSRs include mstatus, misa, mie, mtvec, mscratch, mepc, mcause,
mtval and mip. Machine identity reports the actual hardware IDs; vendor ID 0
is unassigned. Linux supplies the friendly Z-Core vendor/model strings.

Twelve fixed 64-bit counters are available on RV32 as paired low/high CSRs:

| Index | Event |
|---|---|
| 0 | Cycles |
| 2 | Retired instructions |
| 3 | Instruction-cache hits |
| 4 | Data-cache hits |
| 5 | Load requests |
| 6 | Store requests |
| 7 | Branch mispredictions |
| 8 | Pipeline flushes |
| 9 | Instruction-cache misses |
| 10 | Memory fetches |
| 11 | Uncached accesses |
| 12 | Data-cache misses |

Machine counters are at `0xb00 + index`, with high halves at `0xb80 + index`.
Read-only User aliases are at `0xc00 + index` and `0xc80 + index`. Linux enables
indices 0 and 2–12 using `mcounteren=0x1ffd`. Counter snapshots read high/low/high
to handle rollover. Hardware counters run continuously: there is no overflow
interrupt or sampling support. Linux perf accounts deltas
for task/system counting.

`make test-rtl` exercises ordinary execution, cache/write-buffer ordering,
traps, privilege/CSR restrictions, atomic operations, FENCE.I and the stalled
branch regression. The actual Image can also be run with `make sim-linux`.

The independent User-mode bank is read-only at Machine CSRs `0xfc0 + index`
and `0xfe0 + index` for low/high halves. It qualifies cycles and execution
events by User privilege, memory events by their MEM-stage privilege and
retired instructions by their WB-stage privilege. The PMU selects this bank
for `exclude_kernel`, exposed as `perf stat -u`. Total counters are unchanged.
Data-cache misses count one cached demand lookup failure per MEM request,
including forwarded misses and cold stores; stalls do not multiply counts.
Instruction-cache hits and misses partition retired instructions, including
FENCE.I. A hit was consumed from the instruction cache and a miss was consumed
from a refill, so their sum equals the retired-instruction count. The User-mode
bank attributes both with the same privilege as retired instructions.
