#!/usr/bin/env python3
"""
ELF to Verilog HEX Converter for Z-Core
Converts RISC-V ELF files to 32-bit word-addressed hex format for $readmemh.

Usage: python elf2hex.py <input.elf> <output.hex> [memory_size_words]
"""

import sys
import subprocess
import tempfile
import os
import struct

def elf_to_hex(elf_path, hex_path, mem_size_words=16384):
    """Convert ELF to 32-bit word hex format."""
    
    # Create temporary binary file
    with tempfile.NamedTemporaryFile(suffix='.bin', delete=False) as tmp:
        bin_path = tmp.name
    
    try:
        # Convert ELF to raw binary
        objcopy_cmd = [
            'riscv64-unknown-elf-objcopy',
            '-O', 'binary',
            elf_path,
            bin_path
        ]
        
        result = subprocess.run(objcopy_cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Error running objcopy: {result.stderr}")
            sys.exit(1)
        
        # Read binary data
        with open(bin_path, 'rb') as f:
            binary_data = f.read()
        
        # Pad to word boundary
        while len(binary_data) % 4 != 0:
            binary_data += b'\x00'
        
        # Convert to 32-bit words (little-endian, as RISC-V is LE)
        num_words = len(binary_data) // 4
        words = struct.unpack(f'<{num_words}I', binary_data)
        
        # Write hex file
        with open(hex_path, 'w') as f:
            for i, word in enumerate(words):
                f.write(f'{word:08X}\n')
            
            # Optionally pad remaining memory with zeros
            # for i in range(num_words, mem_size_words):
            #     f.write('00000000\n')
        
        print(f"Converted {elf_path} -> {hex_path}")
        print(f"  {num_words} words ({len(binary_data)} bytes)")
        
    finally:
        # Clean up temp file
        if os.path.exists(bin_path):
            os.remove(bin_path)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python elf2hex.py <input.elf> <output.hex> [memory_size_words]")
        sys.exit(1)
    
    elf_file = sys.argv[1]
    hex_file = sys.argv[2]
    mem_size = int(sys.argv[3]) if len(sys.argv) > 3 else 16384
    
    elf_to_hex(elf_file, hex_file, mem_size)
