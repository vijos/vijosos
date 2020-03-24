#ifndef _VM_H_
#define _VM_H_

#include "stdint.h"

#define VM_R 0x1
#define VM_W 0x2
#define VM_X 0x4
#define VM_U 0x8

#include "arch/mm.h"

extern pte_t *kernel_pt;

pte_t *create_pt();
pte_t *get_pte(pte_t *pt, uintptr_t va);
void *map_page(pte_t *pt, uintptr_t va, uint64_t flags);

#endif
