#!/usr/bin/env python3
"""Reset the Z-Core board, upload the current Linux image, and check for login."""
import argparse
from datetime import datetime
import os
from pathlib import Path
import shlex
import shutil
import signal
import subprocess
import sys

import boot

ROOT = Path(__file__).resolve().parents[1]
BITSTREAM = ROOT / "output_files/Z-Core.sof"
IMAGES = ROOT / "software/linux/build/images"
QUARTUS = shutil.which("quartus_pgm") or str(
    Path.home() / "altera_lite/25.1std/quartus/bin/quartus_pgm")

def run_logged(command, logfile):
    print(shlex.join(map(str, command)), flush=True)
    with logfile.open("wb") as log:
        child = subprocess.Popen(command, stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT, start_new_session=True)
        completed = False
        try:
            while True:
                data = os.read(child.stdout.fileno(), 4096)
                if not data:
                    break
                log.write(data)
                log.flush()
                sys.stdout.buffer.write(data)
                sys.stdout.flush()
            result = child.wait()
            completed = True
            return result
        finally:
            if not completed:
                try:
                    os.killpg(child.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
                try:
                    child.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(child.pid, signal.SIGKILL)
                    child.wait()
            child.stdout.close()

def upload_with_recovery(command, logdir, programmer, bitstream, cable,
                         reprogram=False):
    def program():
        if not bitstream.is_file() or not shutil.which(programmer):
            raise RuntimeError("FPGA recovery needs quartus_pgm and the matching "
                               "bitstream; use --quartus-pgm /path and --bitstream /path")
        return run_logged([programmer, "-c", cable, "-m", "JTAG",
                           "-o", f"p;{bitstream}"], logdir / "reprogram.log")

    if reprogram and program():
        return 1
    logfile = logdir / "boot-run.log"
    result = run_logged(command, logfile)
    output = logfile.read_bytes()

    if (result and not reprogram
            and b"Error: no sync response from bootloader." in output
            and b"Sync       : OK" not in output):
        logfile.rename(logdir / "boot-run-attempt1.log")
        print("No bootloader sync; reloading the matching FPGA bitstream "
              "and retrying once.", flush=True)
        if program():
            return 1
        result = run_logged(command, logfile)
    return result

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--images", type=Path, default=IMAGES)
    parser.add_argument("--port", default="/dev/ttyUSB0")
    parser.add_argument("--capture", type=int, default=120,
                        help="seconds to observe Linux after upload (default: 120)")
    parser.add_argument("--log-dir", type=Path,
                        help="new directory for exact image copies and logs")
    parser.add_argument("--reprogram", action="store_true",
                        help="reload the FPGA bitstream before the UART reset")
    parser.add_argument("--bitstream", type=Path, default=BITSTREAM)
    parser.add_argument("--quartus-pgm", default=QUARTUS)
    parser.add_argument("--cable", default="USB-Blaster")
    parser.add_argument("--dry-run", action="store_true",
                        help="validate and show the command without touching hardware")
    args = parser.parse_args(argv)
    if args.capture < 1:
        parser.error("--capture must be positive")
    images = args.images.resolve()
    try:
        manifest = boot.validate(images)
    except (OSError, ValueError) as error:
        parser.error(str(error))
    print(f"Image SHA256: {manifest[0]['sha256']}", flush=True)

    def command(directory):
        return [sys.executable, str(ROOT / "scripts/boot.py"),
                "--images", str(directory), "--uploader", str(ROOT / "scripts/upload.py"),
                "--port", args.port, "--reset-line", "break",
                "--execute", "--capture", str(args.capture)]

    if args.dry_run:
        print(shlex.join(command(images)))
        print(f"FPGA {'reload before upload' if args.reprogram else 'fallback on failed sync'}: "
              f"{args.bitstream.resolve()}")
        print("Dry run: no serial access, reset, FPGA programming or upload.")
        return 0
    fuser = shutil.which("fuser")
    if not fuser:
        parser.error("fuser is required to check UART ownership")
    if not Path(args.port).exists():
        parser.error(f"serial port does not exist: {args.port}")
    owners = subprocess.run([fuser, args.port], capture_output=True, text=True)
    if owners.returncode != 1:
        parser.error(f"UART is busy or cannot be checked: {owners.stdout.strip()} "
                     "Exit your console with Ctrl-] and retry.")

    logdir = (args.log_dir or ROOT / "logs/reboot" /
              datetime.now().strftime("%Y%m%d-%H%M%S-%f")).resolve()
    logdir.mkdir(parents=True, exist_ok=False)
    snapshot = logdir / "images"
    snapshot.mkdir()
    for name in ("Image", "vmlinux", "entry.bin", "zcore.dtb"):
        shutil.copy2(images / name, snapshot / name)
    boot.validate(snapshot)
    print(f"Logs: {logdir}\nWatch: tail -f {shlex.quote(str(logdir / 'boot-run.log'))}",
          flush=True)
    result = upload_with_recovery(command(snapshot), logdir, args.quartus_pgm,
                                  args.bitstream.resolve(), args.cable, args.reprogram)
    if result:
        return result
    if b"zcore login:" not in (snapshot / "uart.log").read_bytes():
        print("Upload succeeded, but Linux did not reach login during capture. "
              "See images/uart.log; UART is released.", file=sys.stderr)
        return 1
    print("Linux reached login. UART released. Connect with:\n"
          f"python3 {shlex.quote(str(ROOT / 'scripts/console.py'))} "
          f"--port {shlex.quote(args.port)} --interactive", flush=True)
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError) as error:
        sys.exit(f"Error: {error}")
    except KeyboardInterrupt:
        sys.exit("Interrupted; UART released.")
