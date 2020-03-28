#ifndef _ARCH_RISCV_TASK_H_
#define _ARCH_RISCV_TASK_H_

#include "stdint.h"

typedef struct
{
    uintptr_t ra;
    uintptr_t sp;
    uintptr_t tp;
    uintptr_t s0;
    uintptr_t s1;
    uintptr_t s2;
    uintptr_t s3;
    uintptr_t s4;
    uintptr_t s5;
    uintptr_t s6;
    uintptr_t s7;
    uintptr_t s8;
    uintptr_t s9;
    uintptr_t s10;
    uintptr_t s11;
} regs_t;

typedef struct
{
    uintptr_t zero;
    uintptr_t ra;
    uintptr_t sp;
    uintptr_t gp;
    uintptr_t tp;
    uintptr_t t0;
    uintptr_t t1;
    uintptr_t t2;
    uintptr_t s0;
    uintptr_t s1;
    uintptr_t a0;
    uintptr_t a1;
    uintptr_t a2;
    uintptr_t a3;
    uintptr_t a4;
    uintptr_t a5;
    uintptr_t a6;
    uintptr_t a7;
    uintptr_t s2;
    uintptr_t s3;
    uintptr_t s4;
    uintptr_t s5;
    uintptr_t s6;
    uintptr_t s7;
    uintptr_t s8;
    uintptr_t s9;
    uintptr_t s10;
    uintptr_t s11;
    uintptr_t t3;
    uintptr_t t4;
    uintptr_t t5;
    uintptr_t t6;
} trap_regs_t;

typedef struct
{
    trap_regs_t regs;
    uintptr_t status;
    uintptr_t epc;
    uintptr_t tval;
    uintptr_t cause;
    uintptr_t cycle;
} trap_frame_t;

#define STACK_REG sp
#define RETURN_ADDR_REG ra
#define THREAD_POINTER_REG tp

static inline struct task *get_current()
{
    register uintptr_t tp asm("tp");
    return (struct task *)tp;
}

static inline void set_current(struct task *task)
{
    asm volatile ("mv tp, %0"
                  :
                  : "r"(task));
}

void arch_return_from_user();

#endif
