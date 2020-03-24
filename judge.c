#include "judge.h"
#include "arch/asm.h"
#include "error.h"
#include "eth.h"
#include "task.h"
#include "string.h"
#include "stdio.h"
#include "elf.h"

__attribute__ ((aligned (16))) uint8_t judge_stack[1024];
static struct task judge_task;
void judge_entry()
{
    while (!get_ip_ready() || ipv4_is_unspecified(get_ifinfo_ipv4()))
    {
        sched_yield();
    }

    puts("Judge Daemon: started!\n");
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
