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

__attribute__ ((aligned (16))) static uint8_t judge_tftp_buff[0x400000];

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
    int ret = tftp_get_file("elf", 3, judge_tftp_buff, sizeof(judge_tftp_buff), &elf_len);
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

    uintptr_t elf_va = USER_TOP - PGSIZE;
    if ((ret = map_and_copy(pt, elf_va, VM_U | VM_R | VM_A, judge_tftp_buff, PGSIZE)) < 0)
    {
        puts("out of memory.\n");
        return ret;
    }

    uintptr_t phdr_va, phent, phnum;
    if ((ret = parse_elf_phdr(judge_tftp_buff, elf_len, &phdr_va, &phent, &phnum)) < 0)
    {
        return ret;
    }
    phdr_va += elf_va;

    uintptr_t stdin_len;
    if ((ret = tftp_get_file("stdin", 5, judge_tftp_buff, sizeof(judge_tftp_buff), &stdin_len)) < 0)
    {
        return ret;
    }

    // stdin buffer
    uintptr_t stdin_va = elf_va - PGALIGN(stdin_len);
    if ((ret = map_and_copy(pt, stdin_va, VM_U | VM_R | VM_A, judge_tftp_buff, stdin_len)) < 0)
    {
        puts("out of memory.\n");
        return ret;
    }

    // stdout buffer
    size_t stdout_len = 0x10000;  // TODO: magic number
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
        Setup stack arguments:
        aux
        NULL
        NULL (auxv[])
        PGSIZE
        AT_PAGESZ (auxv[])
        phnum
        AT_PHNUM (auxv[])
        phent
        AT_PHENT (auxv[])
        phdr_va
        AT_PHDR (auxv[])
        NULL (envp[])
        NULL (argv[])
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

    const uintptr_t stack_word[] =
    {
        0, 0, 0, AT_PHDR, phdr_va, AT_PHENT, phent, AT_PHNUM, phnum, AT_PAGESZ, PGSIZE, 0, 0
    };
    stack_va -= sizeof(stack_word);
    stack -= sizeof(stack_word);
    memcpy(stack, stack_word, sizeof(stack_word));

    printf("Judge Daemon: Entering user-mode.\n");
    //uintptr_t freq = hwtimer_get_freq();
    hwtimer_set_oneshot(req->time_limit + req->time_limit / 10 + 2000000); // +10%
    arch_call_user(pt, entry_va, stack_va);
    resp->exitcode = user_task.exitcode;
    resp->time_usage = user_end_time - user_start_time;
    resp->mem_usage = count_pt(pt, VM_U | VM_A) << PGSHIFT >> 10;
    printf("Judge Daemon: Left user-mode, exitcode %d, %lu cycles, %lu KiB.\n",
           resp->exitcode, resp->time_usage, resp->mem_usage);

    // Upload stdout.
    if (aux->stdout_len > stdout_len)
    {
        aux->stdout_len = stdout_len;
    }
    ctx_t ctx;
    ctx.pt = pt;
    ctx.buff = stdout_va;
    ctx.len = aux->stdout_len;
    ret = tftp_put_file("stdout", 6, judge_read_block, &ctx);
    if (ret < 0)
    {
        return ret;
    }

    resp->seq = req->seq;
    printf("Judge Daemon: Done task %lu.\n", resp->seq);
    return 0;
}

__attribute__ ((aligned (16))) static uint8_t judge_stack[1024];
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

    while (true)
    {
        ipv6_addr_t saddr;
        uint16_t sport;
        size_t ret = recv_udp(JUDGE_SERVER_PORT, &saddr, &sport, &req, sizeof(req),
                              ipv6_unspecified(), 0, 1 * CLOCK_FREQ);
        if (ret == 0)
        {
            continue;
        }

        if (ret != sizeof(req))
        {
            printf("Judge Daemon: Bad request.\n");
            continue;
        }

        memset(&resp, 0, sizeof(resp));
        resp.seq = req.seq;
        resp.flags = JUDGE_RESP_ACK;
        send_udp(JUDGE_SERVER_PORT, saddr, sport, &resp, sizeof(resp));

        mm_take_snapshot();
        resp.error = do_judge(&req, &resp);
        mm_restore_snapshot();

        resp.flags = JUDGE_RESP_ACK | JUDGE_RESP_RESULT;
        send_udp(JUDGE_SERVER_PORT, saddr, sport, &resp, sizeof(resp));
    }
}

void init_judge()
{
    create_task(&judge_task,
                (uintptr_t)judge_stack + sizeof(judge_stack) + STACK_OFFSET,
                judge_entry);
}
