PREFIX  = riscv32-unknown-elf-
CC      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

ARCH    = -march=rv32im_zicsr -mabi=ilp32
CFLAGS  = $(ARCH) -Os -Wall -ffunction-sections -fdata-sections -nostdlib -ffreestanding
LDFLAGS = $(ARCH) -nostdlib -Wl,--gc-sections -T sdram.lds

TARGET  = sdram_test

all: $(TARGET).bin

$(TARGET).elf: start.S
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	$(OBJDUMP) -d $< > $(TARGET).lst
	@echo "Binary size: $$(wc -c < $@) bytes"
	@echo "Upload with:"
	@echo "  python3 ../upload.py /dev/ttyUSB0 $@ --base 0x10000000"

clean:
	rm -f $(TARGET).elf $(TARGET).bin $(TARGET).lst

.PHONY: all clean
