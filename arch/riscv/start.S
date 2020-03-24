.section .text.start, "ax", @progbits
.global _start
_start:
    # TODO: check mhartid
    la t0, trap_entry
    csrw mtvec, t0
    la sp, _end_of_stack - 0x10
    j start

.section .text, "ax", @progbits
.balign 8
trap_entry:
    # reuse main stack now
    la sp, _end_of_stack - 0x10
    call trap_handler
    1:
    j 1b

.section .bss
.balign 16
_stack:
    .space 0x2000
_end_of_stack:
