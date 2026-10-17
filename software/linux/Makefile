.DEFAULT_GOAL := help
ROOT := $(abspath .)
BUILDROOT := $(ROOT)/sources/buildroot-2025.02.1
LINUX := $(ROOT)/sources/linux-6.12.10
CROSS_COMPILE ?= $(ROOT)/build/buildroot/host/bin/riscv32-buildroot-linux-uclibc-
JOBS ?= 4
.PHONY: help fetch configure userspace kernel-check images boot-plan test
help:
	@echo 'fetch        download and verify pinned source archives'
	@echo 'configure    configure Buildroot for RV32IMA/ILP32 no-MMU'
	@echo 'userspace    build toolchain, BusyBox and Linux with initramfs'
	@echo 'kernel-check compile standalone kernel using CROSS_COMPILE (no rootfs)'
	@echo 'images       collect full Buildroot Image and build DTB/entry shim'
	@echo 'boot-plan    validate images and print upload command (no hardware access)'
	@echo 'test         run host tests'
fetch:
	python3 scripts/fetch.py
configure:
	$(MAKE) -C $(BUILDROOT) O=$(ROOT)/build/buildroot BR2_EXTERNAL=$(ROOT) zcore_defconfig
userspace:
	$(MAKE) -C $(BUILDROOT) O=$(ROOT)/build/buildroot -j$(JOBS)
kernel-check:
	python3 scripts/prepare-kernel.py $(LINUX)
	mkdir -p build/linux
	cp configs/linux.config build/linux/.config
	$(MAKE) -C $(LINUX) O=$(ROOT)/build/linux ARCH=riscv CROSS_COMPILE=$(CROSS_COMPILE) olddefconfig
	$(MAKE) -C $(LINUX) O=$(ROOT)/build/linux ARCH=riscv CROSS_COMPILE=$(CROSS_COMPILE) -j$(JOBS) Image
images:
	python3 scripts/verify-build.py build/buildroot
	mkdir -p build/images
	cp build/buildroot/images/Image build/images/Image
	cp build/buildroot/build/linux-6.12.10/vmlinux build/images/vmlinux
	dtc -I dts -O dtb -o build/images/zcore.dtb board/zcore/zcore.dts
	$(CROSS_COMPILE)gcc -march=rv32ima_zicsr_zifencei -mabi=ilp32 -nostdlib -nostartfiles -Wl,--build-id=none -T board/zcore/entry.ld -o build/images/entry.elf board/zcore/entry.S
	$(CROSS_COMPILE)objcopy -O binary build/images/entry.elf build/images/entry.bin
	python3 ../../scripts/boot.py --images build/images
boot-plan:
	python3 ../../scripts/boot.py --images build/images
test:
	$(MAKE) -C ../.. test
