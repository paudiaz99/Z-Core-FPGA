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
ARCH = -march=rv32im_zicsr -mabi=ilp32
CFLAGS = $(ARCH) -O2 -Wall -Wextra -ffreestanding -nostdlib
ASFLAGS = $(ARCH)

# Use linker_app.ld when building for bootloader upload (make APP=1 hello.bin)
ifdef APP
LDFLAGS = -T linker_app.ld
else
LDFLAGS = -T linker.ld
endif

# Source files
SRCS = $(wildcard *.c)
PROGS = $(filter-out uart,$(SRCS:.c=))

# Output files lists
BINS = $(PROGS:=.bin)
HEXS = $(PROGS:=.hex)
MIFS = $(PROGS:=.mif)
ASMS = $(PROGS:=.s)
LSTS = $(PROGS:=.lst)
ELFS = $(PROGS:=.elf)
MAPS = $(PROGS:=.map)

# ================================================================
# Build Targets
# ================================================================

.PHONY: all clean info

all: $(BINS) $(HEXS) $(MIFS) $(ASMS) $(LSTS) info

# Update: uart is now in libs/
UART_DIR = libs
CFLAGS += -I$(UART_DIR)

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
	python3 elf2hex.py $< $@ 4096

# Generate mif file (Quartus format)
%.mif: %.elf
	@echo "Creating mif file $@..."
	python3 elf2hex.py $< $@ 4096

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

# Compile UART from libs
uart.o: $(UART_DIR)/uart.c
	@echo "Compiling UART..."
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
	rm -f *.o *.elf *.bin *.hex *.mif *.s *.lst *.map
