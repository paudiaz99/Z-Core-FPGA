OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 4K
}

SECTIONS
{
    . = 0x00000000;
    
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
    
    /* Stack grows down from top of 4KB RAM */
    _stack_top = 0x00001000;
}
