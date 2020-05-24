#include "syscall.h"

int main()
{
    for (int i = 0; i < 1 << 20; ++i) asm volatile ("");
    return 0;
}
