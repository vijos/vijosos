.section .text.start, "ax", @progbits
.global _start
_start:
    # TODO: check mhartid
    csrw mscratch, zero
    la t0, trap_entry
    csrw mtvec, t0
    la sp, _end_of_stack - 0x10
    j start

.section .bss
.balign 16
_stack:
    .space 0x2000
_end_of_stack:
