#!/usr/bin/env python3
"""Install the version-specific board port. Idempotent, fail on source drift."""
import argparse
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]

def replace(path, old, new):
    text = path.read_text()
    if new in text:
        return
    if text.count(old) != 1:
        raise SystemExit(f"Unsupported source in {path}: expected one anchor")
    path.write_text(text.replace(old, new))

def main():
    p = argparse.ArgumentParser()
    p.add_argument("kernel", type=Path)
    args = p.parse_args()
    k = args.kernel.resolve()
    version = (k / "Makefile").read_text()
    if not all(s in version.splitlines() for s in
               ("VERSION = 6", "PATCHLEVEL = 12", "SUBLEVEL = 10")):
        p.error("this port is pinned to Linux 6.12.10")
    old_soc = '\tdepends on NONPORTABLE && ARCH_RV32I && RISCV_M_MODE && !SMP\n\tselect RISCV_INTC\n'
    new_soc = old_soc + '\tselect PERF_EVENTS\n'
    if old_soc in (k / 'arch/riscv/Kconfig').read_text() and new_soc not in (k / 'arch/riscv/Kconfig').read_text():
        replace(k / 'arch/riscv/Kconfig', old_soc, new_soc)
    replace(k / "arch/riscv/Kconfig", 'config PAGE_OFFSET\n\thex\n',
            'config SOC_ZCORE\n\tbool "Z-Core DE10-Lite"\n'
            '\tdepends on NONPORTABLE && ARCH_RV32I && RISCV_M_MODE && !SMP\n'
            '\tselect RISCV_INTC\n\tselect PERF_EVENTS\n\n'
            'config PAGE_OFFSET\n\thex\n\tdefault 0x10000000 if SOC_ZCORE\n')

    replace(k / "arch/riscv/Kconfig", 'config THREAD_SIZE_ORDER\n',
            'config ARCH_FORCE_MAX_ORDER\n\tint\n'
            '\tdefault 13 if SOC_ZCORE\n\tdefault 10\n\n'
            'config THREAD_SIZE_ORDER\n')
    replace(k / "drivers/tty/serial/Kconfig", 'menu "Serial drivers"',
            'config SERIAL_ZCORE\n\tbool "Z-Core polling UART"\n'
            '\tdepends on SOC_ZCORE\n\tselect SERIAL_CORE\n'
            '\tselect SERIAL_CORE_CONSOLE\n\tselect SERIAL_EARLYCON\n\n'
            'menu "Serial drivers"')
    for relative, line in (
        ("drivers/tty/serial/Makefile", "obj-$(CONFIG_SERIAL_ZCORE) += zcore-uart.o"),
        ("drivers/clocksource/Makefile", "obj-$(CONFIG_SOC_ZCORE) += timer-zcore.o"),
        ("drivers/perf/Makefile", "obj-$(CONFIG_SOC_ZCORE) += pmu-zcore.o"),
        ("drivers/misc/Makefile", "obj-$(CONFIG_SOC_ZCORE) += vga-zcore.o"),
    ):
        path = k / relative
        text = path.read_text()
        if line not in text.splitlines():
            path.write_text(text + "\n" + line + "\n")
    shutil.copyfile(ROOT / "drivers/zcore-uart.c", k / "drivers/tty/serial/zcore-uart.c")
    shutil.copyfile(ROOT / "drivers/timer-zcore.c", k / "drivers/clocksource/timer-zcore.c")
    shutil.copyfile(ROOT / "drivers/pmu-zcore.c", k / "drivers/perf/pmu-zcore.c")
    shutil.copyfile(ROOT / "drivers/vga-zcore.c", k / "drivers/misc/vga-zcore.c")
    replace(k / 'arch/riscv/kernel/cpu.c',
            '\tseq_printf(m, "mvendorid\\t: 0x%lx\\n", ci->mvendorid);',
            '\tif (IS_ENABLED(CONFIG_SOC_ZCORE)) {\n'
            '\t\tseq_puts(m, "vendor_id\\t: Z-Core\\nmodel name\\t: Z-Core RV32IMA\\n");\n'
            '\t}\n\tseq_printf(m, "mvendorid\\t: 0x%lx\\n", ci->mvendorid);')

if __name__ == "__main__":
    main()
