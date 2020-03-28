#ifndef _TASK_H_
#define _TASK_H_

#include "stdint.h"
#include "arch/task.h"
#include "timer.h"
#include "ip.h"

#define NUM_TASKS 10

typedef struct
{
    bool is_waiting;
    uint16_t dport;
    ipv6_addr_t expected_saddr;
    uint16_t expected_sport;
    ipv6_addr_t *saddr;
    uint16_t *sport;
    void *buff;
    size_t len;
    size_t *out_len;
    uint64_t start;
    uint64_t delta;
} wait_packet_t;

struct task
{
    regs_t regs;
    uint64_t wait;
    timer_t timer;
    wait_packet_t wait_packet;
};

typedef struct
{
    int32_t exitcode;
    int32_t error;
    uint64_t error_value;
} user_task_t;

#define WAIT_TTY_RECV (1 << 0)
#define WAIT_TTY_SEND (1 << 1)
#define WAIT_ETH_RECV (1 << 2)
#define WAIT_ETH_SEND (1 << 3)
#define WAIT_EXIT (1ul << 63)

struct task *switch_to(struct task *curr, struct task *next);

static inline struct task *get_current();
static inline void set_current(struct task *task);
#define current (get_current())

extern struct task *const idle_task;

void init_task();
void create_task(struct task *task, uintptr_t stack, void *entry);
void add_task(struct task *task);
void idle_entry();

void sched_yield();
void wait(uint64_t w);
void sleep(uint64_t delta);
void exit();

void task_dispatch_udp(uint16_t dport, ipv6_addr_t saddr, uint16_t sport, void *buff, size_t len);

#endif
