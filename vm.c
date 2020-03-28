#include "vm.h"
#include "mm.h"
#include "arch/mmu.h"
#include "utils.h"
#include "string.h"
#include "stdio.h"

pte_t *kernel_pt = NULL;

pte_t *create_pt()
{
    uintptr_t pg = get_free_page();
    if (!pg) return NULL;

    pte_t *new_pt = p2v(pg);
    if (kernel_pt)
    {
        memcpy(new_pt, kernel_pt, PGSIZE);
    }
    else
    {
        memset(new_pt, 0, PGSIZE);
    }
    return new_pt;
}

static pte_t *get_pt_level(int level, pte_t *pt, uintptr_t idx)
{
    if (!PTE_VALID(pt[idx]))
    {
        uintptr_t new_pt = get_free_page();
        if (!new_pt) return NULL;
        memset(p2v(new_pt), 0, PGSIZE);
        pt[idx] = make_pte(level, new_pt, 0);
        return (pte_t *)p2v(new_pt);
    }
    else
    {
        return (pte_t *)p2v(PTE_PPN(pt[idx]));
    }
}

pte_t *get_pte(pte_t *pt, uintptr_t va)
{
    if (!pt) return NULL;
    if (PGLEVEL >= 4) pt = get_pt_level(3, pt, PT_IDX(3, va));
    if (!pt) return NULL;
    if (PGLEVEL >= 3) pt = get_pt_level(2, pt, PT_IDX(2, va));
    if (!pt) return NULL;
    if (PGLEVEL >= 2) pt = get_pt_level(1, pt, PT_IDX(1, va));
    if (!pt) return NULL;
    return &pt[PT_IDX(0, va)];
}

void *map_page(pte_t *pt, uintptr_t va, uint64_t flags)
{
    pte_t *pte = get_pte(pt, va);
    if (!pte) return NULL;
    if (!PTE_VALID(*pte))
    {
        uintptr_t new_page = get_free_page();
        if (!new_page) return NULL;
        memset(p2v(new_page), 0, PGSIZE);
        *pte = make_pte(0, new_page, flags);
        return (void *)p2v(new_page);
    }
    else
    {
        return (void *)p2v(PTE_PPN(*pte));
    }
}

static size_t count_pt_level(int level, pte_t *pt, uint64_t flags)
{
    size_t count = 0;
    for (size_t i = 0; i < PGSIZE / sizeof(pte_t); ++i)
    {
        if (PTE_VALID(pt[i]))
        {
            if (level > 0 && PTE_TABLE(pt[i]))
            {
                count += count_pt_level(level - 1, (pte_t *)p2v(PTE_PPN(pt[i])), flags);
            }
            else if ((pt[i] & flags) == flags)
            {
                count += 1ul << (level * PGLEVEL_SHIFT);
            }
        }
    }
    return count;
}

size_t count_pt(pte_t *pt, uint64_t flags)
{
    return count_pt_level(PGLEVEL - 1, pt, pte_flags(flags));
}
