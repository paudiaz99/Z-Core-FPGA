#!/usr/bin/env python3
"""Check the final no-MMU build contract before packaging board images."""
import argparse
from pathlib import Path
import struct

def config(path):
    return dict(line.split("=", 1) for line in path.read_text().splitlines()
                if line and not line.startswith("#") and "=" in line)

def require(values, expected, path):
    for key, value in expected.items():
        actual = values.get(key, "n")
        if actual != value:
            raise ValueError(f"{path}: {key}={actual}, expected {value}")

def check(output):
    br = output / ".config"
    require(config(br), {
        "BR2_RISCV_32": "y", "BR2_RISCV_USE_MMU": "n",
        "BR2_RISCV_ISA_RVM": "y", "BR2_RISCV_ISA_RVA": "y",
        "BR2_RISCV_ISA_RVC": "n", "BR2_RISCV_ISA_RVF": "n",
        "BR2_RISCV_ISA_RVD": "n", "BR2_RISCV_ISA_RVV": "n",
        "BR2_RISCV_ABI_ILP32": "y", "BR2_BINFMT_FLAT": "y",
        "BR2_TARGET_ROOTFS_INITRAMFS": "y",
    }, br)
    kernel = output / "build/linux-6.12.10/.config"
    values = config(kernel)
    require(values, {
        "CONFIG_MMU": "n", "CONFIG_ARCH_RV32I": "y", "CONFIG_RISCV_M_MODE": "y",
        "CONFIG_SMP": "n", "CONFIG_FPU": "n", "CONFIG_RISCV_ISA_C": "n",
        "CONFIG_RISCV_ISA_V": "n", "CONFIG_SOC_ZCORE": "y",
        "CONFIG_PAGE_OFFSET": "0x10000000", "CONFIG_SERIAL_ZCORE": "y",
        "CONFIG_BINFMT_FLAT": "y", "CONFIG_HIGH_RES_TIMERS": "y",
        "CONFIG_ARCH_FORCE_MAX_ORDER": "13",
    }, kernel)
    if values.get("CONFIG_INITRAMFS_SOURCE", '""') == '""':
        raise ValueError("kernel configuration has no embedded initramfs")
    busybox = (output / "target/bin/busybox").read_bytes()
    if len(busybox) < 64 or busybox[:4] != b"bFLT":
        raise ValueError("BusyBox must be bFLT, not an ordinary glibc ELF executable")
    revision, = struct.unpack_from(">I", busybox, 4)
    if revision != 4:
        raise ValueError("unexpected bFLT revision")
    init = output / "target/init"
    if not init.is_file() or not init.stat().st_mode & 0o111:
        raise ValueError("missing executable /init")
    cpio = (output / "images/rootfs.cpio").read_bytes()
    if not cpio.startswith((b"070701", b"070702")) or busybox not in cpio:
        raise ValueError("rootfs.cpio does not contain the built BusyBox")
    print(f"Verified RV32IMA/ILP32 no-MMU kernel, BusyBox bFLT ({len(busybox)} bytes), and rootfs.cpio")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    try:
        check(args.output)
    except (ValueError, OSError) as exc:
        parser.error(str(exc))
if __name__ == "__main__":
    main()
