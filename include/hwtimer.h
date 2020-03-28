#ifndef _HWTIMER_H_
#define _HWTIMER_H_

#include "stdint.h"

typedef struct
{
    void (*clear)();
    uintptr_t (*get)();
    uintptr_t (*get_freq)();
    void (*set_oneshot)(uintptr_t t);
} hwtimer_ops_t;

extern const hwtimer_ops_t *hwtimer_ops;

void init_hwtimer();

static inline void hwtimer_clear()
{
    hwtimer_ops->clear();
}

static inline uintptr_t hwtimer_get()
{
    return hwtimer_ops->get();
}

static inline uintptr_t hwtimer_get_freq()
{
    return hwtimer_ops->get_freq();
}

static inline void hwtimer_set_oneshot(uintptr_t t)
{
    hwtimer_ops->set_oneshot(t);
}

#endif
