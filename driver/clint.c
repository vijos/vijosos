#include "driver/clint.h"
#include "arch/asm.h"
#include "hwtimer.h"

void *CLINT_BASE;

void init_clint()
{
    hwtimer_ops = &clint_hwtimer_ops;
}

static void clear()
{
}

static uintptr_t get()
{
    return CLINT_TIME;
}

static uintptr_t get_freq()
{
    return CLOCK_FREQ; // FIXME
}

static void set_oneshot(uintptr_t t)
{
    CLINT_TIMECMP = CLINT_TIME + t;
}

const hwtimer_ops_t clint_hwtimer_ops =
{
    .clear = clear,
    .get = get,
    .get_freq = get_freq,
    .set_oneshot = set_oneshot
};
