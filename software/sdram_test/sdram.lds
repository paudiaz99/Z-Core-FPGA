/* Minimal SDRAM linker script for Z-Core test programs */
MEMORY
{
    SDRAM (rwx) : ORIGIN = 0x10000000, LENGTH = 0x00900000
}
ENTRY(_start)
SECTIONS {
    __stacktop = 0x10900000;
    .text : {
        *(.text.start)
        *(.text .text*)
        *(.rodata .rodata*)
        . = ALIGN(4);
    } > SDRAM
    .data : {
        . = ALIGN(4);
        *(.data .data* .sdata .sdata*)
        . = ALIGN(4);
    } > SDRAM
    .bss : {
        . = ALIGN(4);
        _sbss = .;
        *(.bss .bss* .sbss .sbss* COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SDRAM
}
