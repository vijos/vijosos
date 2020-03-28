#include "mm.h"
#include "arch/mmu.h"
#include "utils.h"
#include "string.h"
#include "stdio.h"

static mm_bitmap_t *mmbm;
static mm_bitmap_t *mmbm_snapshot;

static size_t mmbm_size()
{
    return sizeof(mm_bitmap_t) + ALIGN(mmbm->size >> 3, 8ul);
}

static size_t mmbm_snapshot_size()
{
    return sizeof(mm_bitmap_t) + ALIGN(mmbm_snapshot->size >> 3, 8ul);
}

void init_mm()
{
    extern uint8_t _bss_end[];
    mmbm = (mm_bitmap_t *)ALIGN((uintptr_t)_bss_end, 0x10ul);
    memset(mmbm, 0, sizeof(mm_bitmap_t));
}

void add_pages(uintptr_t start, size_t n)
{
    uintptr_t last_added = mmbm->size;
    start >>= PGSHIFT;
    if (start < last_added)
    {
        printf("Error: Adding previous added pages.\n");
        while (true);
    }
    mmbm->size = start + n;

    uintptr_t last_nblocks = last_added >> 6;
    uintptr_t nblocks = mmbm->size >> 6;
    memset(&mmbm->bits[last_nblocks], 0, (nblocks - last_nblocks) * sizeof(mmbm->bits[0]));
    // Take [last_added, start)
    take_pages(last_added << PGSHIFT, start - last_added);
}

void take_pages(uintptr_t start, size_t n)
{
    uintptr_t start_pg = start >> PGSHIFT;
    for (size_t i = 0; i < n; ++i)
    {
        uintptr_t pg = i + start_pg;
        size_t index = pg >> 6;
        uint64_t mask = 1ul << (pg & 63);
        if (!(mmbm->bits[index] & mask))
        {
            mmbm->bits[index] |= mask;
            ++mmbm->used;
        }
    }
}

void finish_add_pages()
{
    mmbm_snapshot = (mm_bitmap_t *)((uintptr_t)mmbm + mmbm_size());
    uintptr_t available_start = PGALIGN((uintptr_t)mmbm_snapshot + mmbm_size());
    take_pages(0, available_start >> PGSHIFT);
}

uintptr_t get_free_page()
{
    if (mmbm->used == mmbm->size)
    {
        return 0;
    }
    size_t nblocks = mmbm->size >> 6;
    size_t index = mmbm->next;
    for (size_t i = 0; i < nblocks; ++i)
    {
        if (mmbm->bits[index] != -1ul)
        {
            size_t offset = bits_find_zero(mmbm->bits[index]);
            mmbm->bits[index] |= 1ul << offset;
            mmbm->next = index;
            ++mmbm->used;
            return ((index << 6) | offset) << PGSHIFT;
        }
        if (index == nblocks - 1) index = 0;
        else ++index;
    }
    return 0;
}

void mm_take_snapshot()
{
    memcpy(mmbm_snapshot, mmbm, mmbm_size());
}

void mm_restore_snapshot()
{
    memcpy(mmbm, mmbm_snapshot, mmbm_snapshot_size());
}

static bool mm_is_in_use(uintptr_t pg)
{
    uintptr_t n = pg >> PGSHIFT;
    size_t index = n >> 6;
    uint64_t mask = 1ul << (n & 63);
    return mmbm->bits[index] & mask;
}

void mm_print()
{
    printf("Memory Status:\n");
    printf("  Metadata is at 0x%016lx\n", (uintptr_t)mmbm);
    printf("  Metadata snapshot is at 0x%016lx\n", (uintptr_t)mmbm_snapshot);
    printf("  Metadata: %lu bytes\n", mmbm_size());
    printf("  Total Memory: % 12lu bytes\n", mmbm->size << PGSHIFT);
    printf("  Used  Memory: % 12lu bytes\n", mmbm->used << PGSHIFT);
    printf("  Free  Memory: % 12lu bytes\n", (mmbm->size - mmbm->used) << PGSHIFT);
    uintptr_t start = 0;
    bool start_in_use = mm_is_in_use(0);
    for (uintptr_t pg = 0; pg < mmbm->size; ++pg)
    {
        bool in_use = mm_is_in_use(pg << PGSHIFT);
        if (in_use != start_in_use)
        {
            printf("  [0x%016lx - 0x%016lx]: %s\n", start << PGSHIFT, (pg << PGSHIFT) - 1,
                   start_in_use ? "In Use" : "Free");
            start = pg;
            start_in_use = in_use;
        }
    }
    printf("  [0x%016lx - 0x%016lx]: %s\n", start << PGSHIFT, (mmbm->size << PGSHIFT) - 1,
           start_in_use ? "In Use" : "Free");
}
