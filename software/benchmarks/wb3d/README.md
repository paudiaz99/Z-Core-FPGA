# wb3d — Z-Core write-buffer benchmark

A memory-intensive fixed-point 3D rasterizer plus four synthetic phases, built
to exercise `rtl/z_core_write_buffer.v` on hardware. Reports cycles, retired
instructions and IPC per phase, and **self-checks every phase with PASS/FAIL**.

## Why a 3D renderer

The write buffer is not fed by stores. It is fed by **dirty-line evictions from
the write-back D-cache** (`control_u`: `data_cache_dirty_writeback_*`). Loading
it means keeping a write working set far larger than the cache, which a
Z-buffered rasterizer does naturally: every pixel is a depth load, a depth store
and a colour store across ~190 KB of buffers against a 32 KB cache.

Drain is the interesting part:

```verilog
drain_wb = (div_stall || full_out || drain_prediction ||
            fence_stall || store_buffer_hazard) && !empty_out && ...
```

`drain_prediction` is `instruction_hit_pred[2]`, which only goes high after the
**I-cache** has hit 4+ times in a row. That is the one *opportunistic* drain;
every other term is a forced drain, i.e. the stall path. So the buffer's value
is a function of I-cache behaviour — which phases 2 and 3 A/B directly.

Rendering targets SDRAM, not the VGA framebuffer: PMA region 4 (VGA) has
`cacheable = 0`, so writes there bypass the D-cache and the write buffer
entirely. Region 5 (SDRAM) is the only cacheable region.

## Phases

| # | Phase | What it exercises |
|---|-------|-------------------|
| 1 | `FILL stream` | 256 KB of sequential stores. `PIX32=1` → one dirty eviction per pixel; `PIX32=0` → one per four (the line is a single word, so four byte-stores are absorbed by the cache) |
| 2 | `RENDER tight` | The scene with `span_body` inlined. Small inner loop → I-cache hits → background draining |
| 2b | `PHYSICS collide` | `NOBJ` cubes integrated, bounced off the box walls, and an O(N²) pairwise AABB pass with elastic resolve. ALU-bound, and **timed separately** — see below |
| 3 | `RENDER bloat` | **Byte-for-byte identical work** dispatched through 256 distinct copies of the span drawer spread over 64 KB. Suppresses `drain_prediction`, leaving only forced drains on `full_out` |
| 4 | `CONFLICT thrash` | Three arrays exactly `SET_STRIDE` apart → 3 lines per set in a 2-way cache → a dirty eviction every iteration |
| 5 | `MERGE tail` | One dirty + two clean lines per set, so every buffer push is the same address back to back and `do_merge` fires |
| 6 | `FORWARD ld-fwd` | Store, force the eviction, load back. The value must survive whether it is forwarded from the buffer or already drained to SDRAM. A stale value is a hard failure |

### On phase 5

`merge_write` compares only `write_buffer_address[tail_pointer]` — the single
most recent entry. Repeated stores to one word therefore never merge: they hit
in the cache and evict once. A merge needs two *consecutive* dirty evictions of
the same address. The phase gets them by pairing one dirty line with two clean
ones in a set (clean evictions never enter the buffer, so every push is the same
address) **and** by looping over the repeat count on the inside, so 16 pushes
land on one set before moving to the next. Nested the other way, the inner loop
would step to a new set every iteration and `do_merge` would never fire.

### On phase 3

The I-cache (`z_core_instr_cache.v`) is direct-mapped, 4096 one-word entries =
16 KB, index `addr[13:2]`. The 256 span copies are 256-byte aligned, so
`span_n` and `span_n+64` sit exactly 16384 apart and collide. Calling them
round-robin evicts each before its next use. The `asm` carrying a distinct
immediate defeats `-fipa-icf`, which would otherwise fold all 256 bodies into
one at `-O2+`.

## Why physics is timed apart

The collision pass is compute-bound and byte-identical in phases 2 and 3.
Charging it to the phase totals would add the same constant to both and drag
the phase 3 / phase 2 ratio toward 1.0, burying the one measurement that
isolates the write buffer. So `render_phase()` accumulates rasterization and
physics into separate counters, and the A/B at the bottom of the report is
rasterization only.

The simulation is fixed-point throughout, so it is bit-exact reproducible —
which is load-bearing, not incidental. Phases 2 and 3 can only render identical
images if the world evolves identically, so `world_reset()` runs at the top of
every render phase. Without it phase 3 would start from wherever phase 2
stopped and the checksum comparison would prove nothing. The collision count is
folded into the render checksum for the same reason.

## Correctness

The forwarding path and the `elem_count` update logic in the write buffer are
new and hand-written, so the checksums matter more than the timings:

- Phases 2 and 3 render the same scene and **must produce identical checksums**.
  A divergence means data was lost or reordered — no golden constant needed.
- Phases 4, 5 and 6 replay their recurrences in registers and compare.
- Phase 6 fails loudly if any load returns a stale value. It asserts the write
  survives the round trip; it does not distinguish a forwarded read from one
  that was already drained.

The rasterizer maths and all six self-checks are also verified on the host (all
PASS); cache and write-buffer behaviour is target-only, by definition.

## Build

```bash
make -f Makefile.zcore                 # default, 32-bit pixel stores
make -f Makefile.zcore PIX32=0         # 8-bit stores: 1/4 the evictions
make -f Makefile.zcore VGA_BLIT=0      # skip the final blit to VGA
```

`-O3` to match the sibling benchmarks; the flag is echoed in the banner so
numbers stay comparable. `-mstrict-align` because the core traps misaligned
accesses (`misalign_load` / `misalign_store`, mcause 4/6).

## Upload & run

From `software/`:

```bash
python3 ../scripts/upload.py /dev/ttyUSB0 benchmarks/wb3d/wb3d.bin \
    --base 0x10000000 --entry 0x10000000 --reset
```

About 78 KB, uploaded at the default 115200 baud. Results print over UART.

Once all timing is done, the world is reset, run forward `VGA_SETTLE` substeps
so the cubes spread out from their start grid, and one **clean** frame is
rendered and blitted to VGA as an eyeball check (`VGA_ANGLE` picks the
rotation; 0 is axis-aligned and looks flat). The timed phases deliberately composite all `FRAMES` rotations into one
buffer without clearing between them — a partly-filled depth buffer is what
keeps the depth test doing real read-modify-write work — so the buffer they
leave behind is not meant to be looked at.

## Tuning

| Macro | Default | Effect |
|---|---|---|
| `NOBJ` | 48 | Cubes. Pair tests grow as N²; triangles and coverage as N |
| `CUBE_HALF` | `FX(0.42)` | Cube half-extent. Smaller → less overdraw, more triangle setup per covered pixel |
| `SUBSTEPS` | 8 | Physics steps per rendered frame |
| `FRAMES` | 4 | Rendered frames per phase |
| `BOX_X/Y/ZMIN/ZMAX` | — | World bounds; tighter box → more collisions |

## Memory map used

| Region | Address | Size |
|---|---|---|
| code + data | `0x1000_0000` | ~78 KB |
| colour buffer | `0x1100_0000` | 64000 B |
| depth buffer | `0x1101_0000` | 128000 B |
| fill buffer | `0x1104_0000` | 256 KB |
| conflict A/B/C | `0x1110_0000` + *n*·16 KB | 16 KB each |
| merge A/B/C | `0x1118_0000` + *n*·16 KB | 16 KB each |
| forward A/B/C | `0x1120_0000` + *n*·16 KB | 16 KB each |

## Keeping it in sync with the RTL

The cache geometry is duplicated as `#define`s at the top of `wb3d.c` with the
derivation in a comment, and a `_Static_assert` pins `SET_STRIDE` to 16384. If
`DATA_CACHE_DEPTH` changes in `z_core_top_model.v`, the assert fires rather than
the aliasing phases silently detuning.
