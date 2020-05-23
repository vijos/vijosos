#ifndef _SYSCALL_H_
#define _SYSCALL_H_

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;


typedef int64_t intptr_t;
typedef uint64_t uintptr_t;
typedef int64_t ptrdiff_t;
typedef uint64_t size_t;
typedef int64_t ssize_t;

typedef long intmax_t;

#define NULL (void *)(0ul)

#define bool _Bool
#define true 1
#define false 0
#define __bool_true_false_are_defined 1

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

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define NFILES 3

#endif
