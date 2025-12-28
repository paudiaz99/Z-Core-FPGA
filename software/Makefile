# Copyright (c) 2025 Pau Díaz Cuesta

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


# ================================================================
# Makefile for Z-Core RISC-V Programs
# ================================================================

# RISC-V Toolchain
PREFIX = riscv32-unknown-elf-
CC = $(PREFIX)gcc
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE = $(PREFIX)size

# Compiler Flags
ARCH = -march=rv32im -mabi=ilp32
CFLAGS = $(ARCH) -O2 -Wall -Wextra -ffreestanding -nostdlib
ASFLAGS = $(ARCH)
LDFLAGS = -T linker.ld

# Source files
SRCS = $(wildcard *.c)
PROGS = $(filter-out uart,$(SRCS:.c=))

# Output files lists
BINS = $(PROGS:=.bin)
HEXS = $(PROGS:=.hex)
ASMS = $(PROGS:=.s)
LSTS = $(PROGS:=.lst)
ELFS = $(PROGS:=.elf)
MAPS = $(PROGS:=.map)

# ================================================================
# Build Targets
# ================================================================

.PHONY: all clean info

all: $(BINS) $(HEXS) $(ASMS) $(LSTS) info

# Link
%.elf: %.o uart.o start.o linker.ld
	@echo "Linking $@..."
	$(LD) $(LDFLAGS) -Map=$*.map $< uart.o start.o -o $@

# Generate binary
%.bin: %.elf
	@echo "Creating binary $@..."
	$(OBJCOPY) -O binary $< $@

# Generate hex file (Verilog format)
%.hex: %.elf
	@echo "Creating hex file $@..."
	$(OBJCOPY) -O verilog $< $@

# Generate assembly file from C source
%.s: %.c
	@echo "Generating assembly $@..."
	$(CC) $(CFLAGS) -o $@ -S $<

# Generate disassembly listing
%.lst: %.elf
	@echo "Creating listing $@..."
	$(OBJDUMP) -d -S $< > $@

# Compile C files
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble assembly files
%.o: %.S
	@echo "Assembling $<..."
	$(CC) $(ASFLAGS) -c $< -o $@

# Show size information
info: $(ELFS)
	@echo ""
	@echo "=== Binary Size ==="
	$(SIZE) $(ELFS)
	@echo ""
	@echo "=== Output Files ==="
	@ls -lh $(BINS) $(HEXS) $(ASMS) $(LSTS) $(MAPS) 2>/dev/null || dir $(BINS) $(HEXS) $(ASMS) $(LSTS) $(MAPS)

# Clean build artifacts
clean:
	@echo "Cleaning..."
	rm -f *.o *.elf *.bin *.hex *.s *.lst *.map
