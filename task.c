#include "task.h"
#include "tty.h"
#include "eth.h"
#include "timer.h"
#include "string.h"

static struct task _idle_task;
struct task *const idle_task = &_idle_task;

static struct task *tasks[NUM_TASKS];
static size_t num_task = 0;
static size_t num_waiting_task = 0;

void init_task()
{
    create_task(idle_task, 0 /* don't care */, 0 /* don't care */);
    set_current(idle_task);
}

void create_task(struct task *task, uintptr_t stack, void *entry)
{
    task->regs.STACK_REG = stack;
    task->regs.RETURN_ADDR_REG = (uintptr_t)entry;
    task->regs.THREAD_POINTER_REG = (uintptr_t)task;
    task->timer.timeout = 1;
    task->wait_packet.is_waiting = false;
    add_task(task);
}

void add_task(struct task *task)
{
    tasks[num_task] = task;
    ++num_task;
}

static struct task *find_task_waiting_for(uint64_t wait)
{
    for (size_t i = 0; i < num_task; ++i)
    {
        if (tasks[i]->wait & wait)
        {
            return tasks[i];
        }
    }
    return 0;
}

static int switch_to_task_waiting_for(uint64_t wait)
{
    struct task *next = find_task_waiting_for(wait);
    if (!next) return 0;
    switch_to(current, next);
    return 1;
}

static bool task_poll_udp(struct task *task)
{
    if (!task->wait_packet.is_waiting) return true;
    if ((rdcycle() - task->wait_packet.start) >= task->wait_packet.delta)
    {
        task->wait_packet.is_waiting = false;
        return true;
    }
    return false;
}

void idle_entry()
{
    size_t rr = 0;
    while (1)
    {
        if (tty_poll_recv()) switch_to_task_waiting_for(WAIT_TTY_RECV);
        if (tty_poll_send()) switch_to_task_waiting_for(WAIT_TTY_SEND);
        if (eth_poll_recv()) switch_to_task_waiting_for(WAIT_ETH_RECV);
        if (eth_poll_send()) switch_to_task_waiting_for(WAIT_ETH_SEND);
        if (num_waiting_task < num_task)
        {
            for (size_t i = 0; i < num_task; ++i)
            {
                struct task *task = tasks[rr];
                ++rr;
                if (rr == num_task) rr = 0;
                if (task != current && !task->wait
                    && poll_timer(&task->timer) && task_poll_udp(task))
                {
                    switch_to(current, task);
                    break;
                }
            }
        }
    }
}

void sched_yield()
{
    switch_to(current, idle_task);
}

void wait(uint64_t w)
{
    current->wait |= w;
    ++num_waiting_task;
    sched_yield();
    --num_waiting_task;
    current->wait &= ~w;
}

void sleep(uint64_t delta)
{
    set_timer(&current->timer, delta);
    sched_yield();
}

void exit()
{
    wait(WAIT_EXIT);
}

static bool task_match_udp(struct task *task, uint16_t dport, ipv6_addr_t saddr, uint16_t sport)
{
    return (task->wait_packet.dport == 0 || task->wait_packet.dport == dport)
           && (ipv6_is_unspecified(task->wait_packet.expected_saddr)
               || ipv6_eq(task->wait_packet.expected_saddr, saddr))
           && (task->wait_packet.expected_sport == 0 || task->wait_packet.expected_sport == sport);
}

static struct task *task_find_udp(uint16_t dport, ipv6_addr_t saddr, uint16_t sport)
{
    for (size_t i = 0; i < num_task; ++i)
    {
        struct task *task = tasks[i];
        if (task->wait_packet.is_waiting && task_match_udp(task, dport, saddr, sport))
        {
            return task;
        }
    }
    return 0;
}

void task_dispatch_udp(uint16_t dport, ipv6_addr_t saddr, uint16_t sport, void *buff, size_t len)
{
    struct task *target_task = task_find_udp(dport, saddr, sport);
    if (!target_task)
    {
        sched_yield();
        target_task = task_find_udp(dport, saddr, sport);
    }
    if (!target_task)
    {
        return;
    }
    target_task->wait_packet.is_waiting = false;
    *target_task->wait_packet.saddr = saddr;
    *target_task->wait_packet.sport = sport;
    size_t copy_len = target_task->wait_packet.len;
    if (len < copy_len) copy_len = len;
    memcpy(target_task->wait_packet.buff, buff, copy_len);
    *target_task->wait_packet.out_len = copy_len;
}
