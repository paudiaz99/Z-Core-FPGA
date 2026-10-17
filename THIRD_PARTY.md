# Third-party components

The root [LICENSE](LICENSE) (MIT) covers **original Z-Core FPGA project code**
only. Vendored and generated third-party material keeps its own terms. This
file is the inventory.

## Clear to redistribute (with notices)

| Component | Path(s) | Terms |
|-----------|---------|--------|
| Alex Forencich verilog-axi | `rtl/axil_interconnect.v`, `rtl/arbiter.v`, `rtl/priority_encoder.v`, `rtl/axi_mem.v` | MIT — retain copyright headers |
| EEMBC CoreMark | `software/benchmarks/coremark/`, `software/linux/userspace/benchmarks/coremark/` | Apache-2.0 + [EEMBC CoreMark AUA](software/benchmarks/coremark/coremark/LICENSE.md) (trademark rules for modified builds / result claims) |
| STREAM | `software/benchmarks/STREAM/`, Linux `…/stream/` | McCalpin terms in `LICENSE.txt`; label modified results |
| BSC pointer-chase (Z-Core port) | `software/benchmarks/ptr_chase/`, `software/linux/userspace/benchmarks/ptr-chase/` | BSD-3-Clause — see `software/benchmarks/ptr_chase/LICENSE` |
| DOOM engine (id Software / ports) | `software/linux/userspace/doom/` (source only) | GPLv2 — `software/linux/userspace/doom/LICENSE` |
| lmbench (selected sources) | `software/linux/userspace/benchmarks/lmbench/` | GPLv2 (`COPYING`) **plus** `COPYING-2` publication restrictions (below) |
| Painter Whetstone (original) | `software/linux/userspace/benchmarks/original/whetstone.c` | © 1998 Painter Engineering — redistribute only **with the full comment block** |
| Whetstone Z-Core ports | `software/benchmarks/whetstone/`, Linux `…/whetstone/whetstone.c` | New Q16.16 code: MIT (see file headers); algorithm lineage noted |
| Dhrystone 2.1 (netlib originals) | `software/linux/userspace/benchmarks/original/dhry*` | Reinhold P. Weicker; historically shared via netlib; **no OSI SPDX grant** — keep provenance files |
| Dhrystone Z-Core ports | `software/benchmarks/dhrystone/`, Linux `…/dhrystone/` | Port code: MIT; algorithm from Weicker 2.1 (see headers) |
| wb3d | `software/benchmarks/wb3d/`, Linux `…/wb3d/` | MIT — original Z-Core benchmark |
| Linux / Buildroot packaging | `software/linux/` scripts, configs, drivers, packages | Meta + GPL-2.0 text in `software/linux/LICENSE`; kernel/Buildroot fetched at build time |

## lmbench result-publication rules

`COPYING-2` overrides conflicting GPL language for **results**:

1. Do not publish results in a public forum or publication if you modified the benchmarks.
2. Do not distribute results for a fee (including ad-supported sites).

Source redistribution of the included lmbench files remains under GPLv2 + those terms. Prefer not to quote “official” lmbench numbers from modified builds in README or papers.

## DOOM IWAD (not in this repository)

`doom1.wad` is **not** part of the GPLv2 engine source and is gitignored.
Obtain the shareware IWAD yourself under id Software’s shareware terms; do not
treat it as MIT or GPL project content.

## Vendor FPGA IP (not under MIT)

| Component | Path(s) | Notes |
|-----------|---------|--------|
| Altera / Intel Quartus-generated PLL & FIFO | `rtl/sdram_pll0.v`, `rtl/Sdram_Control/Sdram_*_FIFO.v` (+ `.qip`) | © Altera — All rights reserved; Quartus / IP license. Wizard black-box / `.ppf` outputs are gitignored; regenerate in Quartus. |
| Terasic-style SDRAM controller | `rtl/Sdram_Control/{Sdram_Control,command,control_interface,sdr_data_path,Sdram_Params.*}` | Board-demo lineage (DE-series / IS42S16320D); **no open redistributable license in-tree**. Not covered by root MIT. |

Prebuilt bitstreams (`*.sof`) and Linux images are gitignored. Prefer build-from-source; shipping a Linux `Image` implies GPL corresponding-source obligations.

## Project originals (MIT)

Unless listed above, RTL under `rtl/z_core_*.v`, most `axil_*.v`, bridges written for this SoC, `software/libs/`, bootloader, scripts, and docs are MIT per [LICENSE](LICENSE) (Pau Díaz Cuesta / project copyright in file headers).
