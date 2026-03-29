OUTPUT_ARCH("riscv")
ENTRY(_start)

/*
 * Linker script for programs loaded by the Z-Core bootloader.
 * Code is placed at 0x1000 (above the 4 KB bootloader region).
 * Available space: 12 KB  (0x1000 – 0x3FFF)
 * Stack grows down from 0x4000.
 */

MEMORY
{
    RAM (rwx) : ORIGIN = 0x00001000, LENGTH = 12K
}

SECTIONS
{
    . = 0x00001000;

    .text : {
        *(.text.start)
        *(.text*)
        *(.rodata*)
    } > RAM

    .data : {
        __data_start = .;
        *(.data*)
        __data_end = .;
    } > RAM

    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > RAM

    . = ALIGN(8);
    _end = .;

    _stack_top = 0x00004000;
}
