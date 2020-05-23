.section .text, "ax", @progbits
.global _start
_start:
    ld a0, 0x0(sp)
    ld a1, 0x8(sp)
    ld a2, 0x10(sp)
    ld a3, 0x18(sp)
    andi sp, sp, ~0xf
    j init

.global exit
exit:
    ecall

