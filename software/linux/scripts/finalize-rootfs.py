#!/usr/bin/env python3
"""Set sufficient stacks and verify applications before embedding the rootfs."""
from pathlib import Path
import struct
import sys

root=Path(sys.argv[1])
names=['htop','lscpu','nano','vim','perf','zcore-counters','doom','coremark',
       'dhrystone','whetstone','stream','ptr-chase','wb3d',
       'stream-quick','ptr-chase-quick',
       'fpga-dhrystone','fpga-whetstone','fpga-stream',
       'fpga-dhrystone-quick','fpga-whetstone-quick','fpga-stream-quick',
       'lmbench-bw_mem','lmbench-lat_mem_rd','lmbench-lat_syscall']
for name in names:
    path=root/'usr/bin'/name
    data=bytearray(path.read_bytes())
    if data[:4]!=b'bFLT' or struct.unpack_from('>I',data,4)[0]!=4:
        raise SystemExit(f'{path}: expected bFLT revision 4')
    if not struct.unpack_from('>I',data,36)[0] & 1:
        raise SystemExit(f'{path}: bFLT must load text and data together in RAM (-elf2flt=-r)')
    struct.pack_into('>I',data,24,65536)
    path.write_bytes(data)
    print(f'{name}: {len(data)} bytes, 64 KiB stack')
