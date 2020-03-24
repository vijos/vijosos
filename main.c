#include "arch/asm.h"
#include "arch/mm.h"
#include "arch.h"
#include "mm.h"
#include "vm.h"
#include "stdio.h"
#include "string.h"
#include "task.h"
#include "eth.h"
#include "ip.h"
#include "dhcp.h"
#include "judge.h"

#include "driver/axiethernet.h"
#include "driver/uartlite.h"
#include "driver/uart16550.h"

#include "arch/csr.h"

void init_bss()
{
    extern uint8_t _bss_begin[];
    extern uint8_t _bss_end[];
    memset(_bss_begin, 0, (uintptr_t)_bss_end - (uintptr_t)_bss_begin);
}

void create_dummy_task(struct task *task, uintptr_t stack, void *entry)
{
    task->regs.STACK_REG = stack;
    task->regs.RETURN_ADDR_REG = (uintptr_t)entry;
    task->regs.THREAD_POINTER_REG = (uintptr_t)task;
}

void start(int hartid, void *dtb)
{
    fence();
    fence_i();

    init_bss();

    init_arch();

    // TODO: probe
    if ((uintptr_t)dtb >= 0x60000000)
    {
        UARTLITE_BASE = p2v(0x60100000ul);
        init_uartlite();
    }
    else
    {
        // QEMU
        UART16550_BASE = p2v(0x10000000ul);
        init_uart16550();
    }
    if ((uintptr_t)dtb >= 0x60000000)
    {
        ETH_BASE = p2v(0x60200000ul);
        DMA_BASE = p2v(0x60300000ul);
        init_axiethernet();
    }

    // TODO: probe
    init_mm();
    add_pages(0x80000000, 0x40000);
    finish_add_pages();

    init_tty();
    init_eth();
    init_task();
    init_ip();

    printf("***********************************************\n"
           "* Vijos: Vijos Isn't Just an Operating System *\n"
           "***********************************************\n");
    mm_print();
    printf("dtb is at 0x%016lx\n", (uintptr_t)dtb);
    printf("rdcycle: %lu\n", rdcycle());
    printf("rdcycle: %lu\n", rdcycle());

    pte_t *pt = create_pt();
    void *va = (void *)0x3884411000ul;
    void *userpg = map_page(pt, (uintptr_t)va, VM_R | VM_W | VM_X);
    printf("userpg = 0x%016lx\n", (uintptr_t)userpg);
    *(uint64_t *)userpg = 0xcafebabefaceb007;
    set_pt(pt);
    write_csr(mstatus, (read_csr(mstatus) | MSTATUS_MPRV) & ~MSTATUS_MPP);
    uint64_t tmp = *(volatile uint64_t *)va;
    *(volatile uint64_t *)va = 0;
    write_csr(mstatus, read_csr(mstatus) & ~MSTATUS_MPRV);
    printf("0x%016lx\n", tmp);
    printf("0x%016lx\n", *(uint64_t *)userpg);

    init_dhcp();
    init_judge();
    idle_entry();
}
