#ifndef _ARCH_RISCV_ASM_H_
#define _ARCH_RISCV_ASM_H_

#include "stdint.h"

static inline void fence()
{
    asm volatile ("fence");
}

static inline void fence_i()
{
    asm volatile ("fence.i");
}

static inline void flush_tlb()
{
    asm volatile ("sfence.vma");
}

static inline void flush_tlb_one(uintptr_t va)
{
    asm volatile ("sfence.vma %0" : : "r"(va));
}

static inline uint64_t rdcycle()
{
    uint64_t v;
    asm volatile ("rdcycle %0" : "=r"(v));
    return v;
}

#define CLOCK_FREQ 125000000ul

#define STACK_OFFSET (0)

#endif
