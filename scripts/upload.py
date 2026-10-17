#!/usr/bin/env python3
"""Upload binary segments to the Z-Core ROM bootloader over UART."""

import sys
import os
import struct
import time
import select
import argparse
import fcntl
import termios

SYNC_REQ = 0x5A
SYNC_ACK = 0xA5
ACK      = 0x06
NAK      = 0x15

FPGA_CLK_HZ = 50_000_000

TIOCMBIS  = 0x5416
TIOCMBIC  = 0x5417
TIOCM_DTR = 0x002
TIOCM_RTS = 0x004

RESET_LINES = {"dtr": TIOCM_DTR, "rts": TIOCM_RTS}

def set_modem_line(fd, bit, asserted):
    fcntl.ioctl(fd, TIOCMBIS if asserted else TIOCMBIC, struct.pack("I", bit))

def release_reset(fd, line=TIOCM_DTR):
    set_modem_line(fd, line, False)

def pulse_break_reset(fd, settle=0.30):
    import termios
    termios.tcsendbreak(fd, 0)
    time.sleep(settle)
    termios.tcflush(fd, termios.TCIOFLUSH)

def pulse_reset(fd, line=TIOCM_DTR, hold=0.05, settle=0.20):

    set_modem_line(fd, line, True)
    time.sleep(hold)
    set_modem_line(fd, line, False)
    time.sleep(settle)

def configure_port(fd, baud):
    import termios

    baud_map = {
        9600:    termios.B9600,
        19200:   termios.B19200,
        38400:   termios.B38400,
        57600:   termios.B57600,
        115200:  termios.B115200,
        230400:  termios.B230400,
        460800:  termios.B460800,
    }
    if baud not in baud_map:
        raise ValueError(f"Unsupported baud rate: {baud}. "
                         f"Supported: {sorted(baud_map.keys())}")
    baud_const = baud_map[baud]

    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[3] = 0
    attrs[4] = baud_const
    attrs[5] = baud_const
    attrs[6][termios.VMIN]  = 1
    attrs[6][termios.VTIME] = 50
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    termios.tcflush(fd, termios.TCIOFLUSH)

    flags = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, flags & ~os.O_NONBLOCK)

def recv_byte(fd, timeout=5.0):
    r, _, _ = select.select([fd], [], [], timeout)
    if not r:
        raise TimeoutError("No response from device")
    data = os.read(fd, 1)
    if len(data) == 0:
        raise TimeoutError("Read returned 0 bytes")
    return data[0]

def write_all(fd, data):
    remaining = memoryview(data)
    while remaining:
        sent = os.write(fd, remaining)
        if sent <= 0:
            raise OSError("Serial write made no progress")
        remaining = remaining[sent:]

def drain(fd, echo=True):
    output = b""
    while True:
        r, _, _ = select.select([fd], [], [], 0.1)
        if not r:
            break
        chunk = os.read(fd, 256)
        if not chunk:
            break
        output += chunk
    if echo and output:
        sys.stdout.buffer.write(output)
        sys.stdout.flush()
    return output

def capture_mode(fd, seconds, out=None):
    print(f"\n--- Program Output ({seconds}s capture) ---")
    buf = b""
    deadline = time.time() + seconds
    while time.time() < deadline:
        r, _, _ = select.select([fd], [], [], min(0.5, max(0.0, deadline - time.time())))
        if r:
            data = os.read(fd, 256)
            if data:
                buf += data
                sys.stdout.buffer.write(data)
                sys.stdout.flush()
    print(f"\n--- Capture complete ({len(buf)} bytes) ---")
    if out:
        with open(out, "wb") as f:
            f.write(buf)
        print(f"--- Saved to {out} ---")
    return buf

def terminal_mode(fd):
    print("\n--- Program Output (Ctrl-C to exit) ---")
    try:
        while True:
            r, _, _ = select.select([fd], [], [], 0.5)
            if r:
                data = os.read(fd, 256)
                if data:
                    sys.stdout.buffer.write(data)
                    sys.stdout.flush()
    except KeyboardInterrupt:
        print("\n--- Disconnected ---")

def fpga_baud_div(baud):
    return FPGA_CLK_HZ // (16 * baud)

def parse_segment(spec):
    if "@" in spec:
        path, addr_str = spec.rsplit("@", 1)
        addr = int(addr_str, 0)
    else:
        path = spec
        addr = None
    return path, addr

def format_size(n):
    if n >= 1024 * 1024:
        return f"{n / (1024*1024):.1f} MB"
    if n >= 1024:
        return f"{n / 1024:.1f} KB"
    return f"{n} B"

def upload(fd, segments, entry_point, upload_baud):


    print("--- Bootloader Output ---")
    time.sleep(0.3)
    drain(fd, echo=True)

    synced = False
    for attempt in range(5):
        write_all(fd, bytes([SYNC_REQ]))
        try:
            resp = recv_byte(fd, timeout=2.0)
            if resp == SYNC_ACK:
                synced = True
                break
        except TimeoutError:
            pass
        drain(fd, echo=True)

    if not synced:
        print("\nError: no sync response from bootloader.")
        return False

    print("\n--- Upload ---")
    print("Sync       : OK")

    if upload_baud and upload_baud != 115200:
        baud_div = fpga_baud_div(upload_baud)
        actual_baud = FPGA_CLK_HZ // (16 * baud_div)
        print(f"Baud switch: {upload_baud} (div={baud_div}, actual={actual_baud})")
        write_all(fd, struct.pack("<I", baud_div))
        resp = recv_byte(fd, timeout=5.0)
        if resp != ACK:
            print(f"Error: baud negotiation failed (0x{resp:02X})")
            return False

        time.sleep(0.1)
        configure_port(fd, upload_baud)
        time.sleep(0.1)

        resp = recv_byte(fd, timeout=5.0)
        if resp != ACK:
            print(f"Error: no ready ACK at new baud (got 0x{resp:02X})")
            return False
        print(f"Baud       : switched OK")
    else:

        write_all(fd, struct.pack("<I", 0))
        resp = recv_byte(fd, timeout=5.0)
        if resp != ACK:
            print(f"Error: baud ACK failed (0x{resp:02X})")
            return False

    seg_count = len(segments)
    write_all(fd, bytes([seg_count]))
    resp = recv_byte(fd, timeout=5.0)
    if resp != ACK:
        print(f"Error: segment count rejected (0x{resp:02X})")
        drain(fd, echo=True)
        return False

    time.sleep(0.05)
    drain(fd, echo=True)

    total_bytes = sum(len(data) for _, data in segments)
    sent_bytes = 0

    for i, (base, data) in enumerate(segments):
        size = len(data)
        checksum = sum(data) & 0xFFFFFFFF

        print(f"Segment {i} : 0x{base:08X}  {format_size(size)}")

        write_all(fd, struct.pack("<II", base, size))
        resp = recv_byte(fd, timeout=5.0)
        if resp != ACK:
            print(f"  Error: header rejected (0x{resp:02X})")
            drain(fd, echo=True)
            return False

        time.sleep(0.05)
        drain(fd, echo=True)

        chunk_size = 4096
        for offset in range(0, size, chunk_size):
            end = min(offset + chunk_size, size)
            write_all(fd, data[offset:end])
            termios.tcdrain(fd)
            sent_bytes += (end - offset)
            pct = 100.0 * sent_bytes / total_bytes
            print(f"\r  Progress : {pct:5.1f}%  ({format_size(sent_bytes)} / {format_size(total_bytes)})",
                  end="", flush=True)
        print()

        write_all(fd, struct.pack("<I", checksum))
        termios.tcdrain(fd)
        resp = recv_byte(fd, timeout=20.0)
        if resp == NAK:
            print(f"  Error: checksum mismatch!")
            time.sleep(0.1)
            drain(fd, echo=True)
            return False
        if resp != ACK:
            print(f"  Error: unexpected response 0x{resp:02X}")
            return False
        print(f"  Checksum : OK (0x{checksum:08X})")

    write_all(fd, struct.pack("<I", entry_point))
    resp = recv_byte(fd, timeout=5.0)
    if resp != ACK:
        print(f"Error: entry point rejected (0x{resp:02X})")
        return False

    time.sleep(0.1)
    drain(fd, echo=True)

    print(f"\nEntry      : 0x{entry_point:08X}")
    print("Upload complete. Program is running.")
    return True

def main():
    parser = argparse.ArgumentParser(
        description="Z-Core Bootloader Upload Tool v2.0 (Multi-Segment + SDRAM)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""\
Examples:
  %(prog)s /dev/ttyUSB0 software/tests/hello.bin --reset
  %(prog)s /dev/ttyUSB0 test.bin --reset --base 0x10000000
  %(prog)s /dev/ttyUSB0 --segments software/linux/build/images/Image@0x10000000 \\
      software/linux/build/images/entry.bin@0x13fe0000 \\
      software/linux/build/images/zcore.dtb@0x13ff0000 --entry 0x13fe0000 --reset
For validated Linux uploads and boot capture, use scripts/reboot.py.
""")

    parser.add_argument("port", help="Serial port (e.g. /dev/ttyUSB0)")
    parser.add_argument("binary", nargs="?", default=None,
                        help="Binary file for single-segment upload")
    parser.add_argument("--base", type=lambda x: int(x, 0), default=0x1000,
                        help="Load address for single-segment mode (default: 0x1000)")
    parser.add_argument("--segments", nargs="+", metavar="FILE@ADDR",
                        help="Multi-segment: file@address pairs")
    parser.add_argument("--entry", type=lambda x: int(x, 0), default=None,
                        help="Entry point address (default: base of first segment)")
    parser.add_argument("--baud", type=int, default=115200,
                        help="Initial baud rate (default: 115200)")
    parser.add_argument("--fast", action="store_true",
                        help="Switch to 460800 baud after sync")
    parser.add_argument("--upload-baud", type=int, default=None,
                        help="Baud rate for data transfer (overrides --fast)")
    parser.add_argument("--no-terminal", "-n", action="store_true",
                        help="Exit after upload instead of monitoring UART")
    parser.add_argument("--reset", action="store_true",
                        help="Reset the board before uploading "
                             "(UART BREAK by default; see doc/HARDWARE.md)")
    parser.add_argument("--reset-line", choices=sorted(RESET_LINES) + ["break"],
                        default="break",
                        help="How to reset: 'break' needs no extra wiring (default), "
                             "'dtr'/'rts' drive the host_rstn pin")
    parser.add_argument("--capture", type=float, metavar="SECONDS",
                        help="After upload, capture UART for N seconds then exit")
    parser.add_argument("--capture-file", metavar="PATH",
                        help="Also write the captured UART output to PATH")
    args = parser.parse_args()

    segments = []
    if args.segments:
        for spec in args.segments:
            path, addr = parse_segment(spec)
            if addr is None:
                print(f"Error: segment '{spec}' must have @address")
                sys.exit(1)
            with open(path, "rb") as f:
                data = f.read()

            while len(data) % 4:
                data += b"\x00"
            segments.append((addr, data))
    elif args.binary:
        with open(args.binary, "rb") as f:
            data = f.read()
        while len(data) % 4:
            data += b"\x00"
        if len(data) == 0:
            print("Error: binary file is empty.")
            sys.exit(1)
        segments.append((args.base, data))
    else:
        parser.print_help()
        sys.exit(1)

    entry_point = args.entry if args.entry is not None else segments[0][0]

    if args.upload_baud:
        upload_baud = args.upload_baud
    elif args.fast:
        upload_baud = 460800
    else:
        upload_baud = 0

    total = sum(len(d) for _, d in segments)
    print("Z-Core Upload Tool v2.0")
    print(f"  Port   : {args.port} @ {args.baud} baud")
    if upload_baud:
        print(f"  Upload : {upload_baud} baud (after sync)")
    print(f"  Total  : {format_size(total)} in {len(segments)} segment(s)")
    for i, (base, data) in enumerate(segments):
        print(f"    [{i}] 0x{base:08X}  {format_size(len(data))}")
    print(f"  Entry  : 0x{entry_point:08X}")
    print()

    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY)
    configure_port(fd, args.baud)

    if args.reset:
        print(f"  Reset  : {args.reset_line}")
        if args.reset_line == "break":
            pulse_break_reset(fd)
        else:
            pulse_reset(fd, RESET_LINES[args.reset_line])
    if args.reset_line != "break":

        release_reset(fd, RESET_LINES[args.reset_line])

    try:
        ok = upload(fd, segments, entry_point, upload_baud)
        if not ok:
            sys.exit(1)

        if args.capture is not None:
            capture_mode(fd, args.capture, args.capture_file)
        elif not args.no_terminal:
            terminal_mode(fd)
    finally:
        os.close(fd)

if __name__ == "__main__":
    main()
