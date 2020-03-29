#ifndef _ARCH_RISCV_VM_H_
#define _ARCH_RISCV_VM_H_

#include "stdint.h"
#include "arch/asm.h"
#include "arch/csr.h"

#define KERNEL_BASE 0x0ul  // we are in m-mode
// #define KERNEL_BASE 0xffffffc000000000ul
#define USER_TOP          0x4000000000ul
#define USER_BOTTOM       0x0000001000ul

#define p2v(x) (void *)((x) + KERNEL_BASE)
#define v2p(x) ((uintptr_t)(x) - KERNEL_BASE)

#define PGSIZE 0x1000ul
#define PGALIGN(x) (((x) + PGSIZE - 1) & ~(PGSIZE - 1))
#define PGALIGN_FLOOR(x) ((x) & ~(PGSIZE - 1))
#define PGSHIFT 12

#define SATP_MODE 0xF000000000000000
#define SATP_ASID 0x0FFFF00000000000
#define SATP_PPN  0x00000FFFFFFFFFFF

#define SATP_MODE_OFF  0x0000000000000000
#define SATP_MODE_SV39 0x8000000000000000
#define SATP_MODE_SV48 0x9000000000000000
#define SATP_MODE_SV57 0xa000000000000000
#define SATP_MODE_SV64 0xb000000000000000

// page table entry (PTE) fields
#define PTE_V     0x001 // Valid
#define PTE_R     0x002 // Read
#define PTE_W     0x004 // Write
#define PTE_X     0x008 // Execute
#define PTE_U     0x010 // User
#define PTE_G     0x020 // Global
#define PTE_A     0x040 // Accessed
#define PTE_D     0x080 // Dirty
#define PTE_SOFT  0x300 // Reserved for Software

#define PTE_PPN_SHIFT 10

#define PTE_VALID(x) ((x) & PTE_V)
#define PTE_ACCESSED(x) ((x) & PTE_A)
#define PTE_DIRTY(x) ((x) & PTE_D)
#define PTE_TABLE(PTE) (((PTE) & (PTE_V | PTE_R | PTE_W | PTE_X)) == PTE_V)

#define PTE_PPN(x) (((x) >> PTE_PPN_SHIFT) << PGSHIFT)

#define PGLEVEL_SHIFT 9

#define PGLEVEL 3

#define PT_IDX(l, x) (((x) >> (PGSHIFT + PGLEVEL_SHIFT * (l))) & 0x1ff)

typedef uint64_t pte_t;

#include "vm.h"

static inline pte_t pte_flags(uint64_t flags)
{
    pte_t pte = 0;
    if (flags & VM_R) pte |= PTE_R;
    if (flags & VM_W) pte |= PTE_W;
    if (flags & VM_X) pte |= PTE_X;
    if (flags & VM_A) pte |= PTE_A;
    if (flags & VM_D) pte |= PTE_D;
    if (flags & VM_U) pte |= PTE_U;
    return pte;
}

static inline pte_t make_pte(int level, uintptr_t ppn, uint64_t flags)
{
    pte_t pte = (ppn >> PGSHIFT) << PTE_PPN_SHIFT;
    if (level == 0)
    {
        if (flags & VM_R) pte |= PTE_R;
        if (flags & VM_W) pte |= PTE_W;
        if (flags & VM_X) pte |= PTE_X;
        if (flags & VM_A) pte |= PTE_A;
        if (flags & VM_D) pte |= PTE_D;
    }
    else
    {
        pte |= PTE_A;
        pte |= PTE_D;
    }
    if (flags & VM_U) pte |= PTE_U;
    pte |= PTE_V;
    return pte;
}

static inline void set_pt(pte_t *pt)
{
    uintptr_t ppn = v2p(pt) >> PGSHIFT;
    write_csr(satp, ppn | SATP_MODE_SV39);
    flush_tlb();
}

#endif
