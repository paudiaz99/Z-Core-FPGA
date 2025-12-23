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
ARCH = -march=rv32i -mabi=ilp32
CFLAGS = $(ARCH) -O2 -Wall -Wextra -ffreestanding -nostdlib
ASFLAGS = $(ARCH)
LDFLAGS = -T linker.ld

# Source files
SRCS = $(wildcard *.c)
PROGS = $(SRCS:.c=)

# Output files lists
BINS = $(PROGS:=.bin)
HEXS = $(PROGS:=.hex)
LSTS = $(PROGS:=.lst)
ELFS = $(PROGS:=.elf)
MAPS = $(PROGS:=.map)

# ================================================================
# Build Targets
# ================================================================

.PHONY: all clean info

all: $(BINS) $(HEXS) $(LSTS) info

# Link
%.elf: %.o start.o linker.ld
	@echo "Linking $@..."
	$(LD) $(LDFLAGS) -Map=$*.map $< start.o -o $@

# Generate binary
%.bin: %.elf
	@echo "Creating binary $@..."
	$(OBJCOPY) -O binary $< $@

# Generate hex file (Verilog format)
%.hex: %.elf
	@echo "Creating hex file $@..."
	$(OBJCOPY) -O verilog $< $@

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
	@ls -lh $(BINS) $(HEXS) $(LSTS) $(MAPS) 2>/dev/null || dir $(BINS) $(HEXS) $(LSTS) $(MAPS)

# Clean build artifacts
clean:
	@echo "Cleaning..."
	rm -f *.o *.elf *.bin *.hex *.lst *.map
