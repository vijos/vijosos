#include "judge.h"
#include "arch.h"
#include "arch/asm.h"
#include "arch/task.h"
#include "mm.h"
#include "vm.h"
#include "error.h"
#include "hwtimer.h"
#include "eth.h"
#include "tftp.h"
#include "task.h"
#include "string.h"
#include "stdio.h"
#include "elf.h"

__attribute__ ((aligned (16))) uint8_t judge_tftp_buff[0x100000];

// TODO
#define STACK_SIZE 0x400000
#define HEAP_SIZE 0x10000//000
#define HEAP_VA 0x10000000

typedef struct
{
    uint8_t *stdin_buff;
    size_t stdin_len;
    uint8_t *stdout_buff;
    size_t stdout_cap;
    size_t stdout_len;
    void *brk;
    void *brk_end;
} aux_t;

typedef struct
{
    size_t seq;
    size_t time_limit;  // clock ticks
    size_t mem_limit;  // unused
} judge_req_t;

typedef struct
{
    size_t seq;
    size_t time_usage;  // clock ticks
    size_t mem_usage;  // KiB
} judge_resp_t;

// read_block context
typedef struct
{
    pte_t *pt;
    uintptr_t buff;
    size_t len;
} ctx_t;

static int judge_read_block(void *ctx, void *buff)
{
    ctx_t *c = (ctx_t *)ctx;
    int len = TFTP_CHUNK_SIZE;
    if (c->len < len)
    {
        len = c->len;
    }
    uint8_t *buff_mapped = (uint8_t *)map_page(c->pt, c->buff, VM_U | VM_R | VM_W)
                           + (c->buff & (PGSIZE - 1));
    memcpy(buff, buff_mapped, len);
    c->buff += len;
    c->len -= len;
    return len;
}

static int do_judge(judge_req_t *req, judge_resp_t *resp)
{
    printf("Judge Daemon: Receive task %lu.\n", req->seq);

    size_t elf_len;
    int ret = tftp_get_file("elf", 4, judge_tftp_buff, sizeof(judge_tftp_buff), &elf_len);
    if (ret < 0)
    {
        return ret;
    }

    pte_t *pt = create_pt();
    uintptr_t entry_va;
    if ((ret = load_elf(pt, judge_tftp_buff, elf_len, &entry_va)) < 0)
    {
        return ret;
    }

    uintptr_t stdin_len;
    ret = tftp_get_file("stdin", 6, judge_tftp_buff, sizeof(judge_tftp_buff), &stdin_len);
    if (ret < 0)
    {
        return ret;
    }

    // stdin buffer
    uintptr_t stdin_va = USER_TOP - PGALIGN(stdin_len);
    ret = map_and_copy(pt, stdin_va, VM_U | VM_R, judge_tftp_buff, stdin_len);
    if (ret < 0)
    {
        puts("out of memory.\n");
        return ret;
    }

    // stdout buffer
    size_t stdout_len = 0x10000;
    uintptr_t stdout_va = stdin_va - stdout_len;
    for (size_t pg = stdout_va; pg < stdout_va + stdout_len; pg += PGSIZE)
    {
        if (!map_page(pt, pg, VM_U | VM_R | VM_W))
        {
            puts("out of memory.\n");
            return -EOOM;
        }
    }

    // TODO: stderr buffer

    uintptr_t stack_va = stdout_va;
    uintptr_t stack_limit = stack_va - STACK_SIZE;
    for (size_t pg = stack_limit; pg < stack_va; pg += PGSIZE)
    {
        if (!map_page(pt, pg, VM_U | VM_R | VM_W))
        {
            puts("out of memory.\n");
            return -EOOM;
        }
    }

    size_t heap_size = HEAP_SIZE;
    uintptr_t heap_va = HEAP_VA;
    for (size_t pg = heap_va; pg < heap_va + heap_size; pg += PGSIZE)
    {
        if (!map_page(pt, pg, VM_U | VM_R | VM_W))
        {
            puts("out of memory.\n");
            return -EOOM;
        }
    }

    /*
        Arguments:
        aux
        NULL (envp[])
        NULL (argv[])
        &aux
        &envp[]
        &argv[]
        0 (argc)
    */

    stack_va -= 0x10;
    uint8_t *stack = (uint8_t *)map_page(pt, stack_va, VM_U | VM_R | VM_W) + PGSIZE - 0x10;

    stack_va -= sizeof(aux_t);
    stack -= sizeof(aux_t);
    aux_t *aux = (aux_t *)stack;
    aux->stdin_buff = (uint8_t *)stdin_va;
    aux->stdin_len = stdin_len;
    aux->stdout_buff = (uint8_t *)stdout_va;
    aux->stdout_cap = stdout_len;
    aux->stdout_len = 0;
    aux->brk = (void *)heap_va;
    aux->brk_end = (void *)(heap_va + heap_size);
    uintptr_t aux_va = stack_va;

    stack_va -= sizeof(void *);
    stack -= sizeof(void *);
    *(void **)stack = NULL;
    uintptr_t envp = stack_va;

    stack_va -= sizeof(void *);
    stack -= sizeof(void *);
    *(void **)stack = NULL;
    uintptr_t argv = stack_va;

    stack_va -= sizeof(uintptr_t);
    stack -= sizeof(uintptr_t);
    *(uintptr_t *)stack = aux_va;

    stack_va -= sizeof(uintptr_t);
    stack -= sizeof(uintptr_t);
    *(uintptr_t *)stack = envp;

    stack_va -= sizeof(uintptr_t);
    stack -= sizeof(uintptr_t);
    *(uintptr_t *)stack = argv;

    stack_va -= sizeof(size_t);
    stack -= sizeof(size_t);
    *(size_t *)stack = 0;

    printf("Judge Daemon: Entering user-mode.\n");
    uintptr_t freq = hwtimer_get_freq();
    hwtimer_set_oneshot(req->time_limit + req->time_limit / 10 + 2000000); // +10%
    arch_call_user(pt, entry_va, stack_va);
    resp->time_usage = user_end_time - user_start_time;
    resp->mem_usage = count_pt(pt, VM_U | VM_A) << PGSHIFT >> 10;
    printf("Judge Daemon: Left user-mode, exitcode %d, %lu cycles, %lu KiB.\n",
           user_task.exitcode, resp->time_usage, resp->mem_usage);

    if (aux->stdout_len > stdout_len)
    {
        aux->stdout_len = stdout_len;
    }
    ctx_t ctx;
    ctx.pt = pt;
    ctx.buff = stdout_va;
    ctx.len = aux->stdout_len;
    ret = tftp_put_file("stdout", 7, judge_read_block, &ctx);
    if (ret < 0)
    {
        return ret;
    }

    resp->seq = req->seq;
    printf("Judge Daemon: Done task %lu.\n", resp->seq);
    printf("stdout: %s\n", map_page(pt, stdout_va, VM_U | VM_R | VM_W));
    return 0;
}

__attribute__ ((aligned (16))) uint8_t judge_stack[1024];
static struct task judge_task;
void judge_entry()
{
    while (!get_ip_ready() || ipv4_is_unspecified(get_ifinfo_ipv4()))
    {
        sched_yield();
    }

    puts("Judge Daemon: started!\n");

    judge_req_t req;
    judge_resp_t resp;

    for (int i = 0; i < 3; ++i)
    {
        req.time_limit = 125000000;
        req.mem_limit = -1;
        mm_take_snapshot();
        do_judge(&req, &resp);
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
