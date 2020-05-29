#ifndef _JUDGE_H_
#define _JUDGE_H_

#include "stdint.h"

#define JUDGE_SERVER_PORT 1234

#define AT_PHDR 3
#define AT_PHENT 4
#define AT_PHNUM 5
#define AT_PAGESZ 6
#define STACK_SIZE 0x400000
#define HEAP_SIZE 0x400000
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
    size_t time_limit;  // clock cycles
    size_t mem_limit;  // unused
} judge_req_t;

#define JUDGE_RESP_ACK 1
#define JUDGE_RESP_RESULT 2

typedef struct
{
    size_t seq;
    uintptr_t flags;
    intptr_t error;
    intptr_t exitcode;
    size_t time_usage;  // clock cycles
    size_t mem_usage;  // KiB
} judge_resp_t;

void init_judge();

#endif
