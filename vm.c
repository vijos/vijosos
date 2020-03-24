#include "vm.h"
#include "mm.h"
#include "arch/mm.h"
#include "utils.h"
#include "string.h"
#include "stdio.h"

pte_t *kernel_pt = NULL;

pte_t *create_pt()
{
    pte_t *new_pt = p2v(get_free_page());
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
    if (PGLEVEL >= 3) pt = get_pt_level(2, pt, PT_IDX(2, va));
    if (PGLEVEL >= 2) pt = get_pt_level(1, pt, PT_IDX(1, va));
    return &pt[PT_IDX(0, va)];
}

void *map_page(pte_t *pt, uintptr_t va, uint64_t flags)
{
    pte_t *pte = get_pte(pt, va);
    if (!PTE_VALID(*pte))
    {
        uintptr_t new_page = get_free_page();
        memset(p2v(new_page), 0, PGSIZE);
        *pte = make_pte(0, new_page, flags);
        return (void *)p2v(new_page);
    }
    else
    {
        return (void *)p2v(PTE_PPN(*pte));
    }
}
