#include "arch/asm.h"
#include "arch/csr.h"
#include "arch/task.h"
#include "arch.h"
#include "error.h"
#include "vm.h"
#include "arch/mmu.h"
#include "task.h"
#include "printf.h"
#include "stdbool.h"

const char *const cause_desc[] =
{
    "Instruction address misaligned",
    "Instruction access fault",
    "Illegal instruction",
    "Breakpoint",
    "Load address misaligned",
    "Load access fault",
    "Store/AMO address misaligned",
    "Store/AMO access fault",
    "Environment call from U-mode",
    "Environment call from S-mode",
    "Reserved",
    "Environment call from M-mode",
    "Instruction page fault",
    "Load page fault",
    "Reserved",
    "Store/AMO page fault"
};

void print_trap_frame(trap_frame_t *tf)
{
    uint64_t cause = tf->cause;
    const char *desc = "Interrupt";
    if ((cause & CAUSE_INT) == 0)
    {
        desc = cause < 16 ? cause_desc[cause] : "Reserved";
    }
    printf("cause = 0x%016lx (%s)\n", cause, desc);
    printf("epc   = 0x%016lx\n", tf->epc);
    printf("tval  = 0x%016lx\n", tf->tval);
    printf("ra    = 0x%016lx\n", tf->regs.ra);
    printf("sp    = 0x%016lx\n", tf->regs.sp);
    printf("gp    = 0x%016lx\n", tf->regs.gp);
    printf("tp    = 0x%016lx\n", tf->regs.tp);
    printf("t0    = 0x%016lx\n", tf->regs.t0);
    printf("t1    = 0x%016lx\n", tf->regs.t1);
    printf("t2    = 0x%016lx\n", tf->regs.t2);
    printf("s0    = 0x%016lx\n", tf->regs.s0);
    printf("s1    = 0x%016lx\n", tf->regs.s1);
    printf("a0    = 0x%016lx\n", tf->regs.a0);
    printf("a1    = 0x%016lx\n", tf->regs.a1);
    printf("a2    = 0x%016lx\n", tf->regs.a2);
    printf("a3    = 0x%016lx\n", tf->regs.a3);
    printf("a4    = 0x%016lx\n", tf->regs.a4);
    printf("a5    = 0x%016lx\n", tf->regs.a5);
    printf("a6    = 0x%016lx\n", tf->regs.a6);
    printf("a7    = 0x%016lx\n", tf->regs.a7);
    printf("s2    = 0x%016lx\n", tf->regs.s2);
    printf("s3    = 0x%016lx\n", tf->regs.s3);
    printf("s4    = 0x%016lx\n", tf->regs.s4);
    printf("s5    = 0x%016lx\n", tf->regs.s5);
    printf("s6    = 0x%016lx\n", tf->regs.s6);
    printf("s7    = 0x%016lx\n", tf->regs.s7);
    printf("s8    = 0x%016lx\n", tf->regs.s8);
    printf("s9    = 0x%016lx\n", tf->regs.s9);
    printf("s10   = 0x%016lx\n", tf->regs.s10);
    printf("s11   = 0x%016lx\n", tf->regs.s11);
    printf("t3    = 0x%016lx\n", tf->regs.t3);
    printf("t4    = 0x%016lx\n", tf->regs.t4);
    printf("t5    = 0x%016lx\n", tf->regs.t5);
    printf("t6    = 0x%016lx\n", tf->regs.t6);
}

void kernel_exception(trap_frame_t *tf)
{
    printf("\n\nKERNEL EXCEPTION:\n");
    print_trap_frame(tf);
    write_csr(mstatus, 0);
    while (true) asm volatile ("wfi");
}

user_task_t user_task;

void trap_handler(trap_frame_t *tf)
{
    if (tf->cause == CAUSE_PF_FETCH || tf->cause == CAUSE_PF_LOAD || tf->cause == CAUSE_PF_STORE)
    {
        uintptr_t ad = PTE_A;
        if (tf->cause == CAUSE_PF_STORE) ad |= PTE_D;
        uintptr_t va = tf->tval;
        pte_t *pte = get_pte(p2v(read_csr(satp) << PGSHIFT), va);
        if (pte && (*pte & (PTE_V | PTE_U)) == (PTE_V | PTE_U))
        {
            if (ad & PTE_A) printf("setting a bit\n");
            if (ad & PTE_D) printf("setting a bit\n");
            *pte |= ad;
            flush_tlb_one(va);
            return;
        }
        else
        {
            // Page fault.
            user_end_time = tf->cycle;
            user_task.exitcode = -1;
            user_task.error = -EPAGEFAULT;
            user_task.error_value = va;
            arch_return_from_user();
        }
    }

    user_end_time = tf->cycle;

    if (!(tf->cause & CAUSE_INT))
    {
        if (tf->status & MSTATUS_MPP)
        {
            kernel_exception(tf);
            return;
        }

        if (tf->cause == CAUSE_ECALL_U)
        {
            // User process exits normally.
            user_task.exitcode = (int32_t)tf->regs.a0;
            user_task.error = 0;
        }
        else
        {
            // Something goes wrong.
            user_task.exitcode = -1;
            user_task.error = -EUSER;
            user_task.error_value = tf->tval;
        }
        arch_return_from_user();
    }
    else
    {
        uintptr_t irq = (tf->cause << 1) >> 1;
        if (irq == IRQ_M_TIMER)
        {
            // Timed out?
            user_task.exitcode = -1;
            user_task.error = -ETIMEOUT;
            arch_return_from_user();
        }

        printf("\n\nUNHANDLED INTERRUPT:\n");
        print_trap_frame(tf);
        write_csr(mstatus, 0);
        while (true) asm volatile ("wfi");
    }
}
