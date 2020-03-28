.section .text.start, "ax", @progbits
.global riscv_enter_user
riscv_enter_user:
    li ra, 0
    li gp, 0
    li tp, 0
    # clear t0, t1, t2 later
    li s0, 0
    li s1, 0
    li a0, 0
    li a1, 0
    li a2, 0
    li a3, 0
    li a4, 0
    li a5, 0
    li a6, 0
    li a7, 0
    li s2, 0
    li s3, 0
    li s4, 0
    li s5, 0
    li s6, 0
    li s7, 0
    li s8, 0
    li s9, 0
    li s10, 0
    li s11, 0
    li t3, 0
    li t4, 0
    li t5, 0
    li t6, 0

    # Flush D-cache.
    la t0, zeros
    la t1, zeros_end
1:
    ld t2, (t0)
    addi t0, t0, 8
    bne t0, t1, 1b

    # 128KiB of nop to flush I-cache.
.rept 0x10000
    nop
.endr

.balign 64
    fence
    sfence.vma
    fence.i

    rdcycle t0
    sd t0, (t1)
    li t0, 0
    li t1, 0
    mret

.section .bss
zeros:
    .space 0x100000
zeros_end:
.global user_start_time
user_start_time:
    .space 0x8
.global user_end_time
user_end_time:
    .space 0x8
