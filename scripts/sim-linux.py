#!/usr/bin/env python3
"""Run a real Linux Image on the FPGA CPU/cache RTL with ideal AXI RAM/MMIO.

This does not model SDRAM/CDC or FPGA timing. No hardware is accessed.
"""
import argparse
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--verilator", default=shutil.which("verilator"))
p.add_argument("--images", type=Path,
               default=ROOT / "software/linux/build/images")
p.add_argument("--cycles", type=int, default=2_000_000_000)
p.add_argument("--latency", type=int, default=3, help="AXI read response delay in cycles")
p.add_argument("--build-only", action="store_true")
p.add_argument("--commands", type=Path, help="paced UART input lines, starting with root login")
args = p.parse_args()
if not args.verilator:
    p.error("install Verilator or supply --verilator PATH")
if args.cycles < 1 or args.latency < 0:
    p.error("cycles must be positive and latency nonnegative")
rtl = ROOT / "rtl"
out = ROOT / "build/linux-sim"
out.mkdir(parents=True, exist_ok=True)
files = sorted(f for f in rtl.glob("z_core_*.v") if f.name not in
               ("z_core_top_model.v", "z_core_32b_timer.v"))
cmd = [args.verilator, "-Wno-fatal", "--unroll-count", "16384",
       "--unroll-stmts", "1000000", "--cc", "--exe", "--top-module", "linux_sim",
       "--Mdir", str(out), "-I" + str(rtl.parent), str(ROOT / "tests/rtl/linux-sim.sv"),
       *map(str, files), str(rtl / "axil_master.v"), str(ROOT / "tests/rtl/linux-sim.cpp")]
with (out / "build.log").open("w") as log:
    subprocess.run(cmd, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
    subprocess.run(["make", "-C", str(out), "-f", "Vlinux_sim.mk", "-j4"],
                   stdout=log, stderr=subprocess.STDOUT, check=True)
if not args.build_only:
    command = [str(out / "Vlinux_sim"), str(args.images.resolve()),
               str(args.latency), str(args.cycles)]
    if args.commands:
        command.append(str(args.commands.resolve()))
    subprocess.run(command, cwd=out, check=True)
