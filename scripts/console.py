#!/usr/bin/env python3
"""Type commands into the Z-Core UART console and record the replies.

Never resets the board. Characters are paced because the UART has only a
one-byte receive register polled by Linux.
"""
import argparse
import os
from pathlib import Path
import select
import sys
import termios
import time
import tty

def configure(fd):
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[3] = 0
    attrs[4] = attrs[5] = termios.B115200
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)

def read_for(fd, seconds, log, until=None):
    buf = b""
    deadline = time.time() + seconds
    while time.time() < deadline:
        r, _, _ = select.select([fd], [], [], 0.1)
        if r:
            data = os.read(fd, 4096)
            buf += data
            sys.stdout.buffer.write(data)
            sys.stdout.flush()
            log.write(data)
            log.flush()
            if until and until.encode() in buf:
                break
    return buf

def interactive(fd, log, char_delay):
    stdin = sys.stdin.fileno()
    saved = termios.tcgetattr(stdin)
    print("Connected to the running board. Ctrl-] exits; input is paced.", flush=True)
    try:
        tty.setraw(stdin)
        while True:
            ready, _, _ = select.select([stdin, fd], [], [])
            if fd in ready:
                data = os.read(fd, 4096)
                if not data:
                    return
                sys.stdout.buffer.write(data)
                sys.stdout.flush()
                log.write(data)
                log.flush()
            if stdin in ready:
                data = os.read(stdin, 4096)
                if not data:
                    return
                for ch in data:
                    if ch == 0x1d:
                        return
                    os.write(fd, bytes([ch]))
                    termios.tcdrain(fd)
                    time.sleep(char_delay)
    finally:
        termios.tcsetattr(stdin, termios.TCSADRAIN, saved)
        print("\r\nDisconnected. The board is still running.", flush=True)

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--port", default="/dev/ttyUSB0")
    p.add_argument("--log", default=str(Path(__file__).resolve().parents[1] / "logs/console.log"))
    p.add_argument("--char-delay", type=float, default=0.03)
    p.add_argument("--wait", type=float, default=5.0, help="seconds to read after each command")
    p.add_argument("--until", help="stop reading a reply early once this text appears")
    p.add_argument("--interactive", action="store_true", help="paced terminal; Ctrl-] exits without resetting")
    p.add_argument("commands", nargs="*", help="lines to send; empty string sends a bare newline")
    args = p.parse_args()
    if args.interactive and not sys.stdin.isatty():
        p.error("--interactive requires a terminal")
    if args.char_delay < 0 or args.wait < 0:
        p.error("char-delay and wait must be nonnegative")
    Path(args.log).parent.mkdir(parents=True, exist_ok=True)
    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY)
    try:
        configure(fd)
        with open(args.log, "ab") as log:
            read_for(fd, 0.5, log)
            for cmd in args.commands or ([] if args.interactive else [None]):
                if cmd is not None:
                    for ch in cmd.encode() + b"\r":
                        os.write(fd, bytes([ch]))
                        termios.tcdrain(fd)
                        time.sleep(args.char_delay)
                read_for(fd, args.wait, log, args.until)
            if args.interactive:
                interactive(fd, log, args.char_delay)
    finally:
        os.close(fd)

if __name__ == "__main__":
    main()
