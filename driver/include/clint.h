#ifndef _DRIVER_CLINT_H_
#define _DRIVER_CLINT_H_

#include "stdint.h"
#include "stdbool.h"
#include "hwtimer.h"

extern void *CLINT_BASE;
#define CLINT_TIME (*(volatile uint64_t *)((uintptr_t)CLINT_BASE + 0xbff8))
#define CLINT_TIMECMP (*(volatile uint64_t *)((uintptr_t)CLINT_BASE + 0x4000))

extern const hwtimer_ops_t clint_hwtimer_ops;
void init_clint();

#endif
