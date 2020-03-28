#include "hwtimer.h"

const hwtimer_ops_t *hwtimer_ops;

void init_hwtimer()
{
    hwtimer_ops->clear();
}
