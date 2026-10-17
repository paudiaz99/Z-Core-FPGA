
RISCVTYPE = riscv32-unknown-elf
CC        = $(RISCVTYPE)-gcc
OBJCOPY   = $(RISCVTYPE)-objcopy
OBJDUMP   = $(RISCVTYPE)-objdump

OUTFLAG   = -o

SW_DIR    = ../../..
LIBS_DIR  = $(SW_DIR)/libs

PORT_CFLAGS = \
    -O2 \
    -g \
    -march=rv32im_zicsr \
    -mabi=ilp32 \
    -ffreestanding \
    -nostdlib \
    -fno-common \
    -std=gnu99 \
    -fno-tree-loop-distribute-patterns \
    -T $(PORT_DIR)/linker_sdram.ld

FLAGS_STR = "$(PORT_CFLAGS)"
CFLAGS    = $(PORT_CFLAGS) \
            -I$(PORT_DIR) \
            -I. \
            -I$(LIBS_DIR) \
            -DFLAGS_STR=\"$(FLAGS_STR)\"

LFLAGS_END = -lgcc

PORT_SRCS = \
    $(PORT_DIR)/core_portme.c \
    $(PORT_DIR)/ee_printf.c \
    $(LIBS_DIR)/crt0.S

PORT_OBJS =

LOAD = echo "Build done — upload ../coremark.bin via scripts/upload.py"
RUN  = echo "Run on hardware"

OEXT = .o
EXE  = .elf

OPATH = ./
MKDIR = mkdir -p
PERL  = /usr/bin/perl

.PHONY: port_prebuild port_postbuild port_postrun port_prerun port_postload port_preload

port_prebuild:

port_postbuild:
	$(OBJCOPY) -O binary coremark$(EXE) ../coremark.bin
	$(OBJDUMP) -d -S coremark$(EXE) > ../coremark.lst
	@echo "Outputs: coremark$(EXE) (ELF for inspection), ../coremark.bin (raw binary), ../coremark.lst (annotated disassembly)"

port_postrun:
port_prerun:
port_postload:
port_preload:
