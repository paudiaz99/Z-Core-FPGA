#!/usr/bin/env python3
"""
Z-Core Bootloader Upload Tool

Sends a compiled binary to the Z-Core RISC-V bootloader over UART.
No external dependencies -- uses only the Python standard library.

Usage:
    ./upload.py <serial_port> <binary_file> [--baud 115200] [--no-terminal]

Examples:
    ./upload.py /dev/ttyUSB0 hello.bin
    ./upload.py /dev/ttyUSB0 hello.bin -n   # upload only, don't monitor
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


def configure_port(fd, baud):
    """Configure serial port: 8N1, raw mode, given baud rate."""
    import termios

    baud_map = {
        9600:   termios.B9600,
        19200:  termios.B19200,
        38400:  termios.B38400,
        57600:  termios.B57600,
        115200: termios.B115200,
    }
    if baud not in baud_map:
        raise ValueError(f"Unsupported baud rate: {baud}")
    baud_const = baud_map[baud]

    attrs = termios.tcgetattr(fd)
    # Raw input
    attrs[0] = 0
    # Raw output
    attrs[1] = 0
    # 8N1, enable receiver, local mode
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    # No local flags
    attrs[3] = 0
    # Baud rate
    attrs[4] = baud_const
    attrs[5] = baud_const
    # VMIN=1, VTIME=50 (5-second timeout in tenths of a second)
    attrs[6][termios.VMIN]  = 1
    attrs[6][termios.VTIME] = 50
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


def upload(port, binary_path, baud, stay_terminal):
    with open(binary_path, "rb") as f:
        data = f.read()

    # Pad to 4-byte boundary
    while len(data) % 4:
        data += b"\x00"

    size = len(data)
    if size == 0:
        print("Error: binary file is empty.")
        sys.exit(1)
    if size > 12288:
        print(f"Error: binary is {size} bytes, max is 12288 (12 KB).")
        sys.exit(1)

    fd = os.open(port, os.O_RDWR | os.O_NOCTTY)
    configure_port(fd, baud)

    print(f"Z-Core Upload Tool")
    print(f"  Port   : {port} @ {baud} baud")
    print(f"  Binary : {binary_path} ({size} bytes)")
    print()

    # Drain any bootloader banner already sitting in the buffer
    print("--- Bootloader Output ---")
    time.sleep(0.3)
    drain(fd, echo=True)

    # Sync handshake (retry a few times)
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
        # Drain any stale data between retries
        drain(fd, echo=True)

    if not synced:
        print("\nError: no sync response from bootloader.")
        os.close(fd)
        sys.exit(1)

    print("\n--- Upload ---")
    print("Sync    : OK")

    # Send size (ACK/NAK is the first byte the bootloader replies with)
    os.write(fd, struct.pack("<I", size))
    resp = recv_byte(fd, timeout=5.0)
    if resp == NAK:
        print("Error: bootloader rejected size.")
        time.sleep(0.1)
        drain(fd, echo=True)
        os.close(fd)
        sys.exit(1)
    if resp != ACK:
        print(f"Error: unexpected response 0x{resp:02X}")
        os.close(fd)
        sys.exit(1)
    # Drain the text that follows ACK (e.g. "RX 889 bytes\r\n")
    time.sleep(0.05)
    drain(fd, echo=True)
    print(f"Size    : {size} bytes accepted")

    # Send data
    checksum = sum(data) & 0xFFFFFFFF
    os.write(fd, data)
    print(f"Data    : sent")

    # Send checksum (ACK/NAK is the first byte back)
    os.write(fd, struct.pack("<I", checksum))
    resp = recv_byte(fd, timeout=5.0)
    if resp == ACK:
        print(f"Checksum: OK (0x{checksum:08X})")
    elif resp == NAK:
        print(f"Error: checksum mismatch!")
        time.sleep(0.1)
        drain(fd, echo=True)
        os.close(fd)
        sys.exit(1)
    else:
        print(f"Error: unexpected response 0x{resp:02X}")
        os.close(fd)
        sys.exit(1)

    # Drain remaining bootloader messages (e.g. "OK! Jumping to ...")
    time.sleep(0.2)
    drain(fd, echo=True)

    print("\nUpload complete. Program is running.")

    if stay_terminal:
        terminal_mode(fd)

    os.close(fd)


def main():
    parser = argparse.ArgumentParser(description="Z-Core Bootloader Upload Tool")
    parser.add_argument("port", help="Serial port (e.g. /dev/ttyUSB0)")
    parser.add_argument("binary", help="Binary file to upload (.bin)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--no-terminal", "-n", action="store_true",
                        help="Exit after upload instead of monitoring UART")
    args = parser.parse_args()

    upload(args.port, args.binary, args.baud, not args.no_terminal)


if __name__ == "__main__":
    main()
