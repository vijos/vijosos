SECTIONS
{
    . = 0x10000;
    ENTRY(_start)
    .text :
    {
        *(.text*)
    }
    _text_end = .;
    . = ALIGN(0x1000);
    .rodata :
    {
        *(.rodata*)
        *(.srodata*)
    }
    . = ALIGN(0x1000);
    .data :
    {
        *(.data*)
        *(.sdata*)
    }
    _bss_begin = .;
    .bss :
    {
        *(.bss*)
        *(.sbss*)
    }
    . = ALIGN(0x10);
    _bss_end = .;
    /DISCARD/ :
    {
        *(.note.gnu.build-id)
    }
}
