MEMORY
{
    SDRAM (rwx) : ORIGIN = 0x10000000, LENGTH = 0x02000000
}

ENTRY(_start)

SECTIONS {
    __stacktop   = 0x11FF0000;
    __heap_limit = 0x11FE0000;

    .text :
    {
        . = ALIGN(4);
        *(.text.start)
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        *(.srodata)
        *(.srodata*)
        . = ALIGN(4);
        _etext = .;
    } > SDRAM

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.sdata)
        *(.sdata*)
        . = ALIGN(4);
        _edata = .;
    } > SDRAM

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(.sbss)
        *(.sbss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SDRAM

    .heap :
    {
        . = ALIGN(4);
        _heap_start = .;
    } > SDRAM
}
