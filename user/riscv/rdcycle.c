#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static inline uint64_t rdcycle()
{
    uint64_t cycle;
    asm volatile ("rdtime %0" : "=r"(cycle));
    return cycle;
}

int main()
{
    uint64_t c1 = rdcycle(), c2 = rdcycle();
    printf("%lu - %lu = %lu\n", c2, c1, c2 - c1);
    return 0;
}
