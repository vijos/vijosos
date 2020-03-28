#include "arch/asm.h"
#include "arch/csr.h"
#include "arch/task.h"
#include "arch/mmu.h"
#include "mm.h"
#include "stdio.h"

#include "driver/clint.h"
#include "driver/axiethernet.h"
#include "driver/dummyeth.h"
#include "driver/uartlite.h"
#include "driver/uart16550.h"

void init_arch()
{
    uintptr_t pmpc = PMP_NAPOT | PMP_R | PMP_W | PMP_X;
    asm volatile ("la t0, 1f\n\t"
                  "csrrw t0, mtvec, t0\n\t"
                  "csrw pmpaddr0, %1\n\t"
                  "csrw pmpcfg0, %0\n\t"
                  ".align 2\n\t"
                  "1: csrw mtvec, t0"
                  : : "r" (pmpc), "r" (-1UL) : "t0");
}

void init_arch_dev(int hartid, void *dtb)
{
    // TODO: probe
    if ((uintptr_t)dtb >= 0x60000000)
    {
        CLINT_BASE = p2v(0x2000000ul);
        init_clint();
        UARTLITE_BASE = p2v(0x60100000ul);
        init_uartlite();
        ETH_BASE = p2v(0x60200000ul);
        DMA_BASE = p2v(0x60300000ul);
        init_axiethernet();
    }
    else
    {
        // QEMU
        CLINT_BASE = p2v(0x2000000ul);
        init_clint();
        UART16550_BASE = p2v(0x10000000ul);
        init_uart16550();
        init_dummyeth();
    }

    // TODO: probe
    init_mm();
    add_pages(0x80000000, 0x40000);
    finish_add_pages();
}
