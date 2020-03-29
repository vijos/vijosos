.section .text, "ax", @progbits
.global _start
_start:
    la t0, str
    ld zero, (t0)
    la t0, buff
    la t0, buff0
    lw a0, (t0)
    ecall

.section .rodata
str:
    .asciz "hello, world"

.section .data
buff0:
    .quad 0xcafebabe00000000

.section .bss
buff:
    .space 0x10000
    .space 0x4000000
