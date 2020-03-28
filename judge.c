#include "judge.h"
#include "arch.h"
#include "arch/asm.h"
#include "arch/task.h"
#include "mm.h"
#include "vm.h"
#include "error.h"
#include "hwtimer.h"
#include "eth.h"
#include "task.h"
#include "string.h"
#include "stdio.h"
#include "elf.h"

__attribute__ ((aligned (16))) uint8_t elf_buff[0x100000];

__attribute__ ((aligned (16))) uint8_t judge_stack[1024];
static struct task judge_task;
void judge_entry()
{
    while (!get_ip_ready() || ipv4_is_unspecified(get_ifinfo_ipv4()))
    {
        sched_yield();
    }

    puts("Judge Daemon: started!\n");

    for (int i = 0; i < 20; ++i)
    {
        size_t elf_len;
        int ret = tftp_get_file("elf", 4, elf_buff, sizeof(elf_buff), &elf_len);
        if (ret < 0)
        {
            continue;
        }
        mm_take_snapshot();
        pte_t *pt = create_pt();
        uintptr_t entry_va;
        if ((ret = load_elf(pt, elf_buff, elf_len, &entry_va)) < 0)
        {
            mm_restore_snapshot();
            continue;
        }
        //uintptr_t va = 0x3884411000ul;
        //void *userpg = map_page(pt, va, VM_R | VM_X | VM_U);
        //printf("userpg = 0x%016lx\n", (uintptr_t)userpg);
        //*(uint64_t *)userpg = 0x7300016000;  // 1: j 1b; nop; ecall
        printf("Judge Daemon: Entering user-mode.\n");
        uintptr_t freq = hwtimer_get_freq();
        hwtimer_set_oneshot(freq + freq / 10 + 2000000); // 1.1s
        arch_call_user(pt, entry_va, 0);
        uintptr_t user_time = user_end_time - user_start_time;
        printf("Judge Daemon: Left user-mode, exitcode %d, %lu cycles, %lu KiB.\n",
               user_task.exitcode, user_time, count_pt(pt, VM_U | VM_A) << PGSHIFT >> 10);
        mm_restore_snapshot();
    }

    while (true)
    {
        sched_yield();
    }
}

void init_judge()
{
    create_task(&judge_task,
                (uintptr_t)judge_stack + sizeof(judge_stack) + STACK_OFFSET,
                judge_entry);
}
