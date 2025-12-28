#!/bin/bash
# Compile and Generate HEX for Z-Core
# Matches the Makefile configuration for consistency

TARGET="${1:-hello}"

# Toolchain configuration (matches Makefile)
PREFIX="riscv32-unknown-elf-"
CC="${PREFIX}gcc"
LD="${PREFIX}ld"
OBJDUMP="${PREFIX}objdump"
SIZE="${PREFIX}size"

# Compiler flags (matches Makefile)
ARCH="-march=rv32im -mabi=ilp32"
CFLAGS="$ARCH -O2 -Wall -Wextra -ffreestanding -nostdlib"
ASFLAGS="$ARCH"
LDFLAGS="-T linker.ld"

# Check for required files
if [ ! -f "linker.ld" ]; then
    echo "ERROR: linker.ld not found"
    exit 1
fi

if [ ! -f "start.S" ]; then
    echo "ERROR: start.S not found"
    exit 1
fi

if [ ! -f "libs/uart.c" ]; then
    echo "ERROR: libs/uart.c not found"
    exit 1
fi

if [ ! -f "$TARGET.c" ]; then
    echo "ERROR: $TARGET.c not found"
    exit 1
fi

echo "=== Building $TARGET ==="

# Step 1: Compile target C file
echo "Compiling $TARGET.c..."
$CC $CFLAGS -c "$TARGET.c" -o "$TARGET.o" || { echo "ERROR: Compilation failed"; exit 1; }

# Step 1.5: Generate assembly file
echo "Generating assembly $TARGET.s..."
$CC $CFLAGS -S "$TARGET.c" -o "$TARGET.s" || { echo "ERROR: Assembly generation failed"; exit 1; }

# Step 2: Compile UART library
echo "Compiling libs/uart.c..."
$CC $CFLAGS -c libs/uart.c -o uart.o || { echo "ERROR: UART compilation failed"; exit 1; }

# Step 3: Assemble startup code
echo "Assembling start.S..."
$CC $ASFLAGS -c start.S -o start.o || { echo "ERROR: Assembly failed"; exit 1; }

# Step 4: Link
echo "Linking $TARGET.elf..."
$LD $LDFLAGS -Map="$TARGET.map" "$TARGET.o" uart.o start.o -o "$TARGET.elf" || { echo "ERROR: Linking failed"; exit 1; }

# Step 5: Generate disassembly listing
echo "Creating listing $TARGET.lst..."
$OBJDUMP -d -S "$TARGET.elf" > "$TARGET.lst"

# Step 6: Generate HEX file
echo "Generating HEX file $TARGET.hex..."
python3 elf2hex.py "$TARGET.elf" "$TARGET.hex" 1024 || { echo "ERROR: HEX generation failed"; exit 1; }

# Step 7: Show size information
echo ""
echo "=== Binary Size ==="
$SIZE "$TARGET.elf"

# Cleanup intermediate object files
rm -f *.o

echo ""
echo "SUCCESS! Created:"
echo "  - $TARGET.elf  (executable)"
echo "  - $TARGET.hex  (for FPGA memory init)"
echo "  - $TARGET.s    (assembly source)"
echo "  - $TARGET.lst  (disassembly listing)"
echo "  - $TARGET.map  (memory map)"
