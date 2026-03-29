OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
    /* Bootloader occupies the first 4 KB of 16 KB RAM */
    BOOT (rwx) : ORIGIN = 0x00000000, LENGTH = 4K
}

SECTIONS
{
    . = 0x00000000;

    .text : {
        *(.text.start)
        *(.text*)
        *(.rodata*)
    } > BOOT

    .data : {
        __data_start = .;
        *(.data*)
        __data_end = .;
    } > BOOT

    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > BOOT

    . = ALIGN(8);
    _end = .;

    _stack_top = 0x00004000;
}
