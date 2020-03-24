#ifndef _TTY_H_
#define _TTY_H_

#include "stdint.h"
#include "stdbool.h"

typedef struct
{
    void (*putchar)(uint8_t c);
    uint8_t (*getchar)();
    bool (*poll_recv)();
    bool (*poll_send)();
} tty_ops_t;

extern const tty_ops_t *tty_ops;

void init_tty();

void putchar(uint8_t c);
uint8_t getchar();
void _putchar(char ch);

void puts(char *s);
void getline(char *buf, size_t len);

static inline int tty_poll_recv()
{
    return tty_ops->poll_recv();
}

static inline int tty_poll_send()
{
    return tty_ops->poll_send();
}

#endif
