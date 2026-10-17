OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
    . = 0x13fe0000;
    .text : { *(.text.entry) *(.text*) }
    /DISCARD/ : { *(.comment) *(.riscv.attributes) }
    ASSERT(SIZEOF(.text) <= 0x10000, "entry exceeds reserved region")
}
