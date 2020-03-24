SECTIONS
{
    PA_BASE = 0x80000000;
    VA_BASE = 0x80000000;
    ENTRY(_start)
    . = PA_BASE;
    .text :
    {
        *(.text.start)
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
