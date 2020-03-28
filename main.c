#include "arch/asm.h"
#include "arch.h"
#include "mm.h"
#include "stdio.h"
#include "string.h"
#include "hwtimer.h"
#include "tty.h"
#include "eth.h"
#include "task.h"
#include "ip.h"
#include "dhcp.h"
#include "judge.h"

void init_bss()
{
    extern uint8_t _bss_begin[];
    extern uint8_t _bss_end[];
    memset(_bss_begin, 0, (uintptr_t)_bss_end - (uintptr_t)_bss_begin);
}

void start(int hartid, void *dtb)
{
    fence();
    fence_i();

    init_bss();

    init_arch();
    init_arch_dev(hartid, dtb);

    init_hwtimer();
    init_tty();
    init_eth();
    init_task();
    init_ip();

    printf("***********************************************\n"
           "* Vijos: Vijos Isn't Just an Operating System *\n"
           "***********************************************\n");
    mm_print();
    printf("dtb is at 0x%016lx\n", (uintptr_t)dtb);
    printf("rdcycle: %lu, mtime: %lu\n", rdcycle(), hwtimer_get());
    printf("rdcycle: %lu, mtime: %lu\n", rdcycle(), hwtimer_get());

    init_dhcp();
    init_judge();
    idle_entry();
}
