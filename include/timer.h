#ifndef _TIMER_H_
#define _TIMER_H_

#include "stdint.h"
#include "arch/asm.h"

typedef struct
{
    uint64_t start;
    uint64_t delta;
    uint64_t timeout;
} timer_t;

static inline void set_timer(timer_t *t, uint64_t delta)
{
    t->start = rdcycle();
    t->delta = delta;
    t->timeout = 0;
}

static inline int poll_timer(timer_t *t)
{
    if (t->timeout) return 1;
    if ((rdcycle() - t->start) >= t->delta)
    {
        t->timeout = 1;
        return 1;
    }
    return 0;
}

#endif
