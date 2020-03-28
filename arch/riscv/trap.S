#include "arch/csr.h"

#define LOAD ld
#define STORE sd
#define REGBYTES 8

.altmacro
.macro SAVE_ALL
    LOCAL _restore_kernel_sp
    LOCAL _save_context

    # If coming from userspace, preserve the user stack pointer and load
    # the kernel stack pointer. If we came from the kernel, scratch
    # will contain 0, and we should continue on the current stack.

    csrrw sp, mscratch, sp
    bnez sp, _save_context

_restore_kernel_sp:
    csrr sp, mscratch
_save_context:
    addi sp, sp, -37 * REGBYTES

    STORE x5, 5*REGBYTES(sp)
    rdcycle t0
    STORE t0, 36*REGBYTES(sp)

    STORE x1, 1*REGBYTES(sp)
    # x2 is sp, save it later
    STORE x3, 3*REGBYTES(sp)
    STORE x4, 4*REGBYTES(sp)
    STORE x6, 6*REGBYTES(sp)
    STORE x7, 7*REGBYTES(sp)
    STORE x8, 8*REGBYTES(sp)
    STORE x9, 9*REGBYTES(sp)
    STORE x10, 10*REGBYTES(sp)
    STORE x11, 11*REGBYTES(sp)
    STORE x12, 12*REGBYTES(sp)
    STORE x13, 13*REGBYTES(sp)
    STORE x14, 14*REGBYTES(sp)
    STORE x15, 15*REGBYTES(sp)
    STORE x16, 16*REGBYTES(sp)
    STORE x17, 17*REGBYTES(sp)
    STORE x18, 18*REGBYTES(sp)
    STORE x19, 19*REGBYTES(sp)
    STORE x20, 20*REGBYTES(sp)
    STORE x21, 21*REGBYTES(sp)
    STORE x22, 22*REGBYTES(sp)
    STORE x23, 23*REGBYTES(sp)
    STORE x24, 24*REGBYTES(sp)
    STORE x25, 25*REGBYTES(sp)
    STORE x26, 26*REGBYTES(sp)
    STORE x27, 27*REGBYTES(sp)
    STORE x28, 28*REGBYTES(sp)
    STORE x29, 29*REGBYTES(sp)
    STORE x30, 30*REGBYTES(sp)
    STORE x31, 31*REGBYTES(sp)

    csrrw t0, mscratch, zero # read old sp to t0, clear scratch
    STORE t0, 2*REGBYTES(sp)

    # get and save status, epc, tval, cause
    csrr t0, mstatus
    STORE t0, 32*REGBYTES(sp)

    csrr t0, mepc
    STORE t0, 33*REGBYTES(sp)

    csrr t0, mtval
    STORE t0, 34*REGBYTES(sp)

    csrr t0, mcause
    STORE t0, 35*REGBYTES(sp)
.endm

.macro RESTORE_ALL
    LOCAL _save_kernel_sp
    LOCAL _restore_context

    LOAD t1, 32*REGBYTES(sp) # saved mstatus
    li t0, MSTATUS_MPP
    and t0, t1, t0
    bnez t0, _restore_context

_save_kernel_sp:
    addi t0, sp, 37 * REGBYTES
    csrw mscratch, t0
_restore_context:
    csrw mstatus, t1

    LOAD t0, 33*REGBYTES(sp)
    csrw mepc, t0

    LOAD x1, 1*REGBYTES(sp)
    # x2 is sp, load it later
    LOAD x3, 3*REGBYTES(sp)
    LOAD x4, 4*REGBYTES(sp)
    LOAD x5, 5*REGBYTES(sp)
    LOAD x6, 6*REGBYTES(sp)
    LOAD x7, 7*REGBYTES(sp)
    LOAD x8, 8*REGBYTES(sp)
    LOAD x9, 9*REGBYTES(sp)
    LOAD x10, 10*REGBYTES(sp)
    LOAD x11, 11*REGBYTES(sp)
    LOAD x12, 12*REGBYTES(sp)
    LOAD x13, 13*REGBYTES(sp)
    LOAD x14, 14*REGBYTES(sp)
    LOAD x15, 15*REGBYTES(sp)
    LOAD x16, 16*REGBYTES(sp)
    LOAD x17, 17*REGBYTES(sp)
    LOAD x18, 18*REGBYTES(sp)
    LOAD x19, 19*REGBYTES(sp)
    LOAD x20, 20*REGBYTES(sp)
    LOAD x21, 21*REGBYTES(sp)
    LOAD x22, 22*REGBYTES(sp)
    LOAD x23, 23*REGBYTES(sp)
    LOAD x24, 24*REGBYTES(sp)
    LOAD x25, 25*REGBYTES(sp)
    LOAD x26, 26*REGBYTES(sp)
    LOAD x27, 27*REGBYTES(sp)
    LOAD x28, 28*REGBYTES(sp)
    LOAD x29, 29*REGBYTES(sp)
    LOAD x30, 30*REGBYTES(sp)
    LOAD x31, 31*REGBYTES(sp)
    # restore sp last
    LOAD x2, 2*REGBYTES(sp)
.endm

.section .text, "ax", @progbits
.balign 64
.global trap_entry
trap_entry:
    SAVE_ALL
    mv a0, sp
    jal trap_handler
    # sp should be the same as before "jal trap_handler"
    # Fall through

.global trap_return
trap_return:
    RESTORE_ALL
    mret  # return from machine call
