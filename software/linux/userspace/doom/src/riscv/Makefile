CROSS ?= riscv32-unknown-elf-

CC = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE = $(CROSS)size

CFLAGS=-std=gnu99 -Wall -O1 -march=rv32im_zicsr -mabi=ilp32 -mstrict-align -ffreestanding -nostdlib -fno-builtin -fno-strict-aliasing -fomit-frame-pointer -Wl,--gc-sections,--no-warn-mismatch -I..
CFLAGS += -Wno-incompatible-pointer-types -Wno-int-conversion

CFLAGS += \
	-DNORMALUNIX \
	-DZCORE_DOOM \
	$(NULL)

include ../sources.mk

SOURCES_doom := $(filter-out d_main.c,$(SOURCES_doom))

SOURCES_doom := $(filter-out s_sound.c,$(SOURCES_doom))

SOURCES_doom_arch := \
	d_main.c \
	i_main.c \
	i_net.c \
	i_sound.c \
	i_system.c \
	i_video.c \
	s_sound.c \
	start.S \
	console.c  \
	libc_backend.c  \
	mini-printf.c \
	nolibc.c \
	$(NULL)

all: doom-zcore.bin

doom-zcore.elf: zcore.lds $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch)
	$(CC) $(CFLAGS) -Wl,-Bstatic,-T,zcore.lds,--strip-debug -o $@ $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch)
	$(SIZE) $@

clean:
	rm -f *.bin *.hex *.elf *.o *.gen.h

%.bin: %.elf
	$(OBJCOPY) -O binary $<  $@

.PHONY: all clean
.PRECIOUS: *.elf
