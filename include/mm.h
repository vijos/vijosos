#ifndef _MM_H_
#define _MM_H_

#include "stdint.h"

typedef struct
{
    uint64_t size;
    uint64_t next;
    uint64_t used;
    uint64_t bits[];
} mm_bitmap_t;

void init_mm();
void add_pages(uintptr_t start, size_t n);
void finish_add_pages();
void take_pages(uintptr_t start, size_t n);
uintptr_t get_free_page();

void mm_take_snapshot();
void mm_restore_snapshot();

void mm_print();

#endif
