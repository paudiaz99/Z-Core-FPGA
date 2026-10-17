OUTPUT_ARCH("riscv")
ENTRY(_start)


MEMORY
{
    SDRAM (rwx) : ORIGIN = 0x10000000, LENGTH = 64M
}

SECTIONS
{
    . = 0x10000000;

    .text : {
        *(.text.start)
        *(.text*)
        *(.rodata*)
    } > SDRAM

    .data : {
        __data_start = .;
        *(.data*)
        *(.sdata*)
        __data_end = .;
    } > SDRAM

    .bss : {
        __bss_start = .;
        *(.bss*)
        *(.sbss*)
        *(COMMON)
        __bss_end = .;
    } > SDRAM

    . = ALIGN(8);
    _end = .;

    _stack_top = 0x14000000;
}
