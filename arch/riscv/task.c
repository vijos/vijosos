#include "arch/task.h"
#include "task.h"
#include "arch/asm.h"
#include "arch/csr.h"
#include "arch/mmu.h"
#include "string.h"

static struct task *last_task;
static struct task user_task;

__attribute__ ((aligned (16))) uint8_t trap_stack[0x1000];
void riscv_enter_user(void);

void arch_call_user(void *pt, uintptr_t pc, uintptr_t sp)
{
    set_pt((pte_t *)pt);
    write_csr(mepc, pc);
    uintptr_t status = MSTATUS_MPIE | (PRV_U << MSTATUS_MPP_SHIFT);
    write_csr(mstatus, status);
    write_csr(mideleg, 0);
    write_csr(medeleg, 0);
    write_csr(mie, MIP_MTIP);

    memset(&user_task.regs, 0, sizeof(user_task.regs));
    user_task.regs.ra = (uintptr_t)riscv_enter_user;
    user_task.regs.sp = sp;
    last_task = current;
    // Set up trap stack.
    write_csr(mscratch, (uintptr_t)trap_stack + sizeof(trap_stack) + STACK_OFFSET);
    switch_to(current, &user_task);
}

void arch_return_from_user()
{
    switch_to(NULL, last_task);
}
