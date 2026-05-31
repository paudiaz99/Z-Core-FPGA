#!/usr/bin/env python3
"""
Z-Core Bootloader Upload Tool v2.0 (Multi-Segment + SDRAM)

Sends one or more binary segments to the Z-Core bootloader over UART.
Supports baud rate negotiation for faster uploads (e.g. 460800 baud).

Usage:
    # Single file (legacy-style, loads to 0x1000 by default):
    ./upload.py /dev/ttyUSB0 hello.bin

    # Single file to SDRAM:
    ./upload.py /dev/ttyUSB0 doom.bin --base 0x10000000

    # Multi-segment (DOOM code + WAD):
    ./upload.py /dev/ttyUSB0 --segments doom.bin@0x10000000 doom1.wad@0x12010000 --entry 0x10000000

    # With high-speed baud:
    ./upload.py /dev/ttyUSB0 --segments doom.bin@0x10000000 doom1.wad@0x12010000 --entry 0x10000000 --fast

No external dependencies -- uses only the Python standard library.
"""

import sys
import os
import struct
import time
import select
import argparse

# Protocol constants
SYNC_REQ = 0x5A
SYNC_ACK = 0xA5
ACK      = 0x06
NAK      = 0x15

# FPGA clock for baud divisor calculation
FPGA_CLK_HZ = 50_000_000


def configure_port(fd, baud):
    """Configure serial port: 8N1, raw mode, given baud rate."""
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
    attrs[0] = 0                                          # Raw input
    attrs[1] = 0                                          # Raw output
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL  # 8N1
    attrs[3] = 0                                          # No local flags
    attrs[4] = baud_const                                 # Input baud
    attrs[5] = baud_const                                 # Output baud
    attrs[6][termios.VMIN]  = 1
    attrs[6][termios.VTIME] = 50                          # 5s timeout
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    termios.tcflush(fd, termios.TCIOFLUSH)

    import fcntl
    flags = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, flags & ~os.O_NONBLOCK)


def recv_byte(fd, timeout=5.0):
    """Read one byte with timeout. Returns int or raises TimeoutError."""
    r, _, _ = select.select([fd], [], [], timeout)
    if not r:
        raise TimeoutError("No response from device")
    data = os.read(fd, 1)
    if len(data) == 0:
        raise TimeoutError("Read returned 0 bytes")
    return data[0]


def drain(fd, echo=True):
    """Read and optionally print all buffered data from the port."""
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


def terminal_mode(fd):
    """Simple terminal: display serial output until Ctrl-C."""
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
    """Compute the FPGA UART baud divisor for a given baud rate."""
    return FPGA_CLK_HZ // (16 * baud)


def parse_segment(spec):
    """Parse 'file@address' or just 'file' (address defaults to None)."""
    if "@" in spec:
        path, addr_str = spec.rsplit("@", 1)
        addr = int(addr_str, 0)
    else:
        path = spec
        addr = None
    return path, addr


def format_size(n):
    """Human-readable byte size."""
    if n >= 1024 * 1024:
        return f"{n / (1024*1024):.1f} MB"
    if n >= 1024:
        return f"{n / 1024:.1f} KB"
    return f"{n} B"


def upload(fd, segments, entry_point, upload_baud):
    """
    Upload segments using the v2 multi-segment protocol.

    segments: list of (base_addr, data_bytes) tuples
    entry_point: address to jump to after upload
    upload_baud: baud rate for data transfer (0 = stay at 115200)
    """
    # --- Drain any bootloader banner ---
    print("--- Bootloader Output ---")
    time.sleep(0.3)
    drain(fd, echo=True)

    # --- Sync handshake ---
    synced = False
    for attempt in range(5):
        os.write(fd, bytes([SYNC_REQ]))
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

    # --- Baud negotiation ---
    if upload_baud and upload_baud != 115200:
        baud_div = fpga_baud_div(upload_baud)
        actual_baud = FPGA_CLK_HZ // (16 * baud_div)
        print(f"Baud switch: {upload_baud} (div={baud_div}, actual={actual_baud})")
        os.write(fd, struct.pack("<I", baud_div))
        resp = recv_byte(fd, timeout=5.0)
        if resp != ACK:
            print(f"Error: baud negotiation failed (0x{resp:02X})")
            return False
        # Switch host baud and wait for bootloader to settle
        time.sleep(0.1)
        configure_port(fd, upload_baud)
        time.sleep(0.1)
        # Wait for ready ACK at new baud
        resp = recv_byte(fd, timeout=5.0)
        if resp != ACK:
            print(f"Error: no ready ACK at new baud (got 0x{resp:02X})")
            return False
        print(f"Baud       : switched OK")
    else:
        # Send 0 = keep current baud
        os.write(fd, struct.pack("<I", 0))
        resp = recv_byte(fd, timeout=5.0)
        if resp != ACK:
            print(f"Error: baud ACK failed (0x{resp:02X})")
            return False

    # --- Segment count ---
    seg_count = len(segments)
    os.write(fd, bytes([seg_count]))
    resp = recv_byte(fd, timeout=5.0)
    if resp != ACK:
        print(f"Error: segment count rejected (0x{resp:02X})")
        drain(fd, echo=True)
        return False
    # Drain text output
    time.sleep(0.05)
    drain(fd, echo=True)

    # --- Send each segment ---
    total_bytes = sum(len(data) for _, data in segments)
    sent_bytes = 0

    for i, (base, data) in enumerate(segments):
        size = len(data)
        checksum = sum(data) & 0xFFFFFFFF

        print(f"Segment {i} : 0x{base:08X}  {format_size(size)}")

        # Send base + size
        os.write(fd, struct.pack("<II", base, size))
        resp = recv_byte(fd, timeout=5.0)
        if resp != ACK:
            print(f"  Error: header rejected (0x{resp:02X})")
            drain(fd, echo=True)
            return False
        # Drain text
        time.sleep(0.05)
        drain(fd, echo=True)

        # Send payload in chunks with progress
        chunk_size = 4096
        for offset in range(0, size, chunk_size):
            end = min(offset + chunk_size, size)
            os.write(fd, data[offset:end])
            sent_bytes += (end - offset)
            pct = 100.0 * sent_bytes / total_bytes
            print(f"\r  Progress : {pct:5.1f}%  ({format_size(sent_bytes)} / {format_size(total_bytes)})",
                  end="", flush=True)
        print()

        # Send checksum
        os.write(fd, struct.pack("<I", checksum))
        resp = recv_byte(fd, timeout=10.0)
        if resp == NAK:
            print(f"  Error: checksum mismatch!")
            time.sleep(0.1)
            drain(fd, echo=True)
            return False
        if resp != ACK:
            print(f"  Error: unexpected response 0x{resp:02X}")
            return False
        print(f"  Checksum : OK (0x{checksum:08X})")

    # --- Entry point ---
    os.write(fd, struct.pack("<I", entry_point))
    resp = recv_byte(fd, timeout=5.0)
    if resp != ACK:
        print(f"Error: entry point rejected (0x{resp:02X})")
        return False
    # Drain text (e.g. "Jump 0x...")
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
  %(prog)s /dev/ttyUSB0 hello.bin                    # BRAM app (0x1000)
  %(prog)s /dev/ttyUSB0 test.bin --base 0x10000000   # Single SDRAM segment
  %(prog)s /dev/ttyUSB0 --segments doom.bin@0x10000000 doom1.wad@0x12010000 --entry 0x10000000
  %(prog)s /dev/ttyUSB0 --segments doom.bin@0x10000000 doom1.wad@0x12010000 --entry 0x10000000 --fast
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
    args = parser.parse_args()

    # Build segment list
    segments = []
    if args.segments:
        for spec in args.segments:
            path, addr = parse_segment(spec)
            if addr is None:
                print(f"Error: segment '{spec}' must have @address")
                sys.exit(1)
            with open(path, "rb") as f:
                data = f.read()
            # Pad to 4-byte boundary
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

    # Determine entry point
    entry_point = args.entry if args.entry is not None else segments[0][0]

    # Determine upload baud
    if args.upload_baud:
        upload_baud = args.upload_baud
    elif args.fast:
        upload_baud = 460800
    else:
        upload_baud = 0  # Stay at initial baud

    # Print summary
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

    # Open port at initial baud
    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY)
    configure_port(fd, args.baud)

    try:
        ok = upload(fd, segments, entry_point, upload_baud)
        if not ok:
            os.close(fd)
            sys.exit(1)

        if not args.no_terminal:
            terminal_mode(fd)
    finally:
        os.close(fd)


if __name__ == "__main__":
    main()
