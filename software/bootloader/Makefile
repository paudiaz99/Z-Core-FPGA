# Makefile for Z-Core Bootloader

PREFIX   = riscv32-unknown-elf-
CC       = $(PREFIX)gcc
AS       = $(PREFIX)as
LD       = $(PREFIX)ld
OBJCOPY  = $(PREFIX)objcopy
OBJDUMP  = $(PREFIX)objdump
SIZE     = $(PREFIX)size

ARCH     = -march=rv32im_zicsr -mabi=ilp32
CFLAGS   = $(ARCH) -Os -Wall -Wextra -ffreestanding -nostdlib -ffunction-sections -fdata-sections -I../libs
ASFLAGS  = $(ARCH)
LDFLAGS  = -T linker_boot.ld --gc-sections

TARGET   = bootloader
MIF_OUT  = ..

OBJS     = boot_start.o bootloader.o uart.o

.PHONY: all clean

all: $(TARGET).elf $(TARGET).bin $(TARGET).lst mif
	@echo ""
	@echo "=== Bootloader Size ==="
	$(SIZE) $(TARGET).elf
	@echo ""

uart.o: ../libs/uart.c
	$(CC) $(CFLAGS) -c $< -o $@

boot_start.o: boot_start.S
	$(CC) $(ASFLAGS) -c $< -o $@

bootloader.o: bootloader.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS) linker_boot.ld
	$(LD) $(LDFLAGS) -Map=$(TARGET).map $(OBJS) -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).lst: $(TARGET).elf
	$(OBJDUMP) -d -S $< > $@

mif: $(TARGET).elf
	python3 ../elf2hex.py $< $(MIF_OUT)/$(TARGET).mif 4096

clean:
	rm -f *.o *.elf *.bin *.lst *.map
	rm -f $(MIF_OUT)/$(TARGET).mif $(MIF_OUT)/$(TARGET)_byte*.mif
