#!/usr/bin/env python3
"""Validate a Linux upload and print its command; hardware is opt-in."""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
RAM = 0x10000000
SHIM = 0x13fe0000
DTB = 0x13ff0000
END = 0x14000000
FPGA = ROOT

def validate(images):
    image = (images / "Image").read_bytes()
    elf = (images / "vmlinux").read_bytes()
    if len(elf) < 52 or elf[:7] != b"\x7fELF\x01\x01\x01":
        raise ValueError("vmlinux must be ELF32 little endian")
    etype, machine = struct.unpack_from("<HH", elf, 16)
    entry, phoff = struct.unpack_from("<II", elf, 24)
    flags, = struct.unpack_from("<I", elf, 36)
    phsize, phnum = struct.unpack_from("<HH", elf, 42)
    if etype != 2 or machine != 243 or entry != RAM or flags & 7:
        raise ValueError("vmlinux must be RV32 soft-float, no C, linked at SDRAM base")
    if phsize != 32 or phnum == 0 or phoff + phsize * phnum > len(elf):
        raise ValueError("invalid ELF program headers")
    loads = 0
    for i in range(phnum):
        kind, offset, va, pa, filesz, memsz, _, _ = struct.unpack_from(
            "<8I", elf, phoff + i * phsize)
        if kind != 1:
            continue
        loads += 1
        if not (RAM <= va < SHIM and filesz <= memsz and va + memsz <= SHIM):
            raise ValueError("ELF load segment overlaps reserved memory")
        if offset + filesz > len(elf) or image[va-RAM:va-RAM+filesz] != elf[offset:offset+filesz]:
            raise ValueError("Image does not match vmlinux load segments")
    if not loads:
        raise ValueError("vmlinux has no load segments")
    if len(image) < 64 or image[56:60] != b"RSC\x05":
        raise ValueError("not a RISC-V Linux Image")
    offset, size = struct.unpack_from("<QQ", image, 8)
    if offset != 0 or not size or max(size, len(image)) > SHIM - RAM:
        raise ValueError("kernel offset/size overlaps reserved boot memory")
    dtb = (images / "zcore.dtb").read_bytes()
    if len(dtb) < 40 or dtb[:4] != b"\xd0\x0d\xfe\xed":
        raise ValueError("invalid DTB header")
    total, = struct.unpack_from(">I", dtb, 4)
    if total != len(dtb) or len(dtb) > END - DTB:
        raise ValueError("invalid DTB size")
    shim = (images / "entry.bin").read_bytes()
    if not shim or len(shim) > DTB - SHIM:
        raise ValueError("invalid entry shim size")
    segments = [("Image", RAM, image), ("entry.bin", SHIM, shim), ("zcore.dtb", DTB, dtb)]
    return [{"file": name, "address": address, "bytes": len(data),
             "sha256": hashlib.sha256(data).hexdigest()} for name, address, data in segments]

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--images", type=Path,
                   default=ROOT / "software/linux/build/images")
    p.add_argument("--fpga-repo", type=Path, default=Path(FPGA))
    p.add_argument("--uploader", type=Path, help="override the uploader script")
    p.add_argument("--port", default="/dev/ttyUSB0")
    p.add_argument("--capture", type=int, default=120)
    p.add_argument("--reset-line", choices=("break", "dtr", "rts"), default="break")
    p.add_argument("--execute", action="store_true", help="actually reset, upload and capture UART")
    args = p.parse_args()
    try:
        manifest = validate(args.images)
    except (OSError, ValueError) as e:
        p.error(str(e))
    if args.capture < 1:
        p.error("capture must be positive")
    images = args.images.resolve()
    uploader = args.uploader or args.fpga_repo / "scripts/upload.py"
    if not uploader.is_file():
        p.error(f"missing uploader: {uploader}")
    cmd = [sys.executable, str(uploader), args.port, "--segments"]
    cmd += [f"{images / item['file']}@{item['address']:#x}" for item in manifest]
    cmd += ["--entry", hex(SHIM), "--reset", "--reset-line", args.reset_line,
            "--capture", str(args.capture), "--capture-file", str(images / "uart.log")]
    print(json.dumps(manifest, indent=2))
    print(shlex.join(cmd), flush=True)
    print("Requires RV32IMA, U-mode, coherent FENCE.I and legal WFI; see doc/HARDWARE.md.", flush=True)
    if args.execute:
        (images / "upload-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
        subprocess.run(cmd, check=True)
    else:
        print("Dry run: no serial port opened, reset or upload performed.")
if __name__ == "__main__":
    main()
