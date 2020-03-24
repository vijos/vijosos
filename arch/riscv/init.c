#include "arch/asm.h"
#include "arch/csr.h"
#include "stdio.h"

void init_arch()
{
    uintptr_t pmpc = PMP_NAPOT | PMP_R | PMP_W | PMP_X;
    asm volatile ("la t0, 1f\n\t"
                  "csrrw t0, mtvec, t0\n\t"
                  "csrw pmpaddr0, %1\n\t"
                  "csrw pmpcfg0, %0\n\t"
                  ".align 2\n\t"
                  "1: csrw mtvec, t0"
                  : : "r" (pmpc), "r" (-1UL) : "t0");
}
