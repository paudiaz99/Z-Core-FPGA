.DEFAULT_GOAL := help
.PHONY: help bootloader examples benchmarks coremark
help:
	@echo 'bootloader / examples / benchmarks / coremark'
	@echo 'Linux sources and build instructions: linux/ and ../doc/LINUX.md'
bootloader:
	$(MAKE) -C bootloader
examples:
	$(MAKE) -C tests APP=1 hello.bin led_test.bin gpio_test.bin multiplication.bin
benchmarks:
	$(MAKE) -C benchmarks/dhrystone -f Makefile.zcore
	$(MAKE) -C benchmarks/whetstone -f Makefile.zcore
	$(MAKE) -C benchmarks/STREAM -f Makefile.zcore
	$(MAKE) -C benchmarks/ptr_chase -f Makefile.zcore
	$(MAKE) -C benchmarks/wb3d -f Makefile.zcore
	$(MAKE) coremark
coremark:
	$(MAKE) -C benchmarks/coremark/coremark PORT_DIR=../z_core compile ITERATIONS=2000
	$(MAKE) -C benchmarks/coremark/coremark PORT_DIR=../z_core port_postbuild
