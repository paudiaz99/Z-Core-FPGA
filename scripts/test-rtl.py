#!/usr/bin/env python3
"""Run CPU/cache regressions on the shipped FPGA RTL without accessing hardware."""
import argparse
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument("suite", nargs="?", choices=("all", "control", "trap", "linux", "stall", "counter", "usercounter"), default="all")
p.add_argument("--cross", default="riscv32-unknown-elf-")
a = p.parse_args()
out = ROOT / "build/rtl-tests"
out.mkdir(parents=True, exist_ok=True)
rtl = ROOT / "rtl"
tests = ROOT / "tests/rtl"
names = {"control": "z_core_control_u_tb", "trap": "z_core_trap_tb",
         "linux": "z_core_linux_tb", "stall": "z_core_stall_tb", "counter": "z_core_counter_tb", "usercounter": "z_core_usercounter_tb"}
sources = [str(f) for f in sorted(rtl.glob("z_core_*.v"))
           if f.name != "z_core_top_model.v"]
sources.extend(str(rtl / name) for name in
               ("arbiter.v", "priority_encoder.v", "axil_master.v",
                "axil_interconnect.v", "axil_uart.v", "axil_gpio.v", "axil_timer.v"))
sources.append(str(tests / "axil_ram.v"))
for suite in (names if a.suite == "all" else [a.suite]):
    name = names[suite]
    base = "z_core_linux_tb" if suite in ("stall", "counter", "usercounter") else name
    text = (tests / f"{base}.sv").read_text()
    if suite in ("stall", "counter", "usercounter"):
        text = text[:text.index("    reg [4095:0] linux_hex;")]
        text = text.replace("module z_core_linux_tb;", "module " + name + ";")
        text += (tests / ("user-counters.svh" if suite == "usercounter" else "dcache-counter.svh" if suite == "counter" else "write-buffer-stall.svh")).read_text() + "\nendmodule\n"
    for old, new in (("valid_bits_0", "valid_bits_way0"), ("valid_bits_1", "valid_bits_way1"),
                     ("tags_0", "tags_way0"), ("tags_1", "tags_way1"),
                     ("data_0", "data_way0"), ("data_1", "data_way1")):
        text = text.replace("data_cache." + old, "data_cache." + new)
    text = text.replace("wait_cycles(5);", "wait_cycles(DATA_CACHE_DEPTH + 5);")
    if suite == "trap":
        text = text.replace("wait_cycles(8);", "wait_cycles(DATA_CACHE_DEPTH + 8);")
    text = text.replace('$dumpfile("z_core_control_u_tb.vcd");', "")
    text = text.replace("$dumpvars(0, z_core_control_u_tb);", "")
    tb = out / f"{suite}.sv"
    tb.write_text(text)
    if suite in ("linux", "counter", "usercounter"):
        compiler = shutil.which(a.cross + "gcc")
        if not compiler:
            p.error("Linux architecture tests need a RISC-V compiler; set --cross PREFIX")
        stem = "user-counters" if suite == "usercounter" else "dcache-counter" if suite == "counter" else "linux-entry-test"
        source = "user-counters.S" if suite == "usercounter" else "dcache-counter.S" if suite == "counter" else "linux-entry.S"
        elf = out / (stem + ".elf")
        subprocess.run([compiler, "-march=rv32ima_zicsr_zifencei", "-mabi=ilp32",
                        "-nostdlib", "-nostartfiles", "-Wl,--build-id=none",
                        "-T", str(tests / "test.ld"), "-o", str(elf),
                        str(tests / source)], check=True)
        subprocess.run(["python3", str(ROOT / "scripts/elf2hex.py"), str(elf),
                        str(out / (stem + ".hex")), "4096"], check=True)
    exe = out / f"{suite}.vvp"
    with (out / f"{suite}-compile.log").open("w") as log:
        subprocess.run(["iverilog", "-g2012", "-I", str(ROOT), "-I", str(rtl),
                        "-s", name, "-o", str(exe), *sources, str(tb)],
                       cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
    command = ["vvp", str(exe)]
    if suite in ("linux", "counter", "usercounter"):
        key = "counter_hex" if suite in ("counter", "usercounter") else "linux_hex"
        command.append(f"+{key}={out / (stem + '.hex')}")
    with (out / f"{suite}.log").open("w") as log:
        subprocess.run(command, cwd=out, stdout=log, stderr=subprocess.STDOUT,
                       check=True, timeout=180)
    result = (out / f"{suite}.log").read_text()
    if any(marker in result for marker in ("[FAIL]", "[PERF-FAIL]", "TIMEOUT", "Timeout!", "SOME TESTS FAILED")):
        raise SystemExit(f"FAILED: {out / (suite + '.log')}")
    print("\n".join(result.splitlines()[-14:]))
