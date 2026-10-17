.DEFAULT_GOAL := help
QUARTUS_SH ?= quartus_sh
JOBS ?= 4
.PHONY: help boot console fpga linux bootloader examples benchmarks test test-rtl sim-linux
help:
	@echo 'boot        program output_files/Z-Core.sof and upload software/linux/build/images'
	@echo 'console     connect to running Linux (Ctrl-] exits)'
	@echo 'fpga        compile the local Quartus project'
	@echo 'linux       fetch pinned sources and rebuild kernel + all applications'
	@echo 'bootloader  rebuild BRAM bootloader and its MIF files'
	@echo 'examples    build small bare-metal examples for UART upload'
	@echo 'benchmarks  build bare-metal benchmarks'
	@echo 'test        run image/recovery/project checks without hardware'
	@echo 'test-rtl    run CPU/cache regressions without hardware'
	@echo 'sim-linux   boot software/linux/build/images in RTL simulation'
boot:
	python3 scripts/reboot.py --reprogram
console:
	python3 scripts/console.py --interactive
fpga:
	$(QUARTUS_SH) --flow compile Z-Core
linux:
	$(MAKE) -C software/linux fetch
	$(MAKE) -C software/linux configure
	$(MAKE) -C software/linux userspace JOBS=$(JOBS)
	$(MAKE) -C software/linux images
bootloader examples benchmarks:
	$(MAKE) -C software $@
test:
	python3 -m unittest discover -s tests -v
test-rtl:
	python3 scripts/test-rtl.py
sim-linux:
	python3 scripts/sim-linux.py
