#include "tty.h"
#include "driver/uartlite.h"

const tty_ops_t *tty_ops;

void init_tty()
{
}

void putchar(uint8_t c)
{
    tty_ops->putchar(c);
}

uint8_t getchar()
{
    return tty_ops->getchar();
}

// for printf.h
void _putchar(char ch) __attribute__((alias("putchar")));

void puts(char *s)
{
    char ch;
    while ((ch = *s++)) putchar(ch);
}

void getline(char *buf, size_t len)
{
    char *buf_end = buf + len - 1;
    while (buf != buf_end)
    {
        *buf = getchar();
        if (*buf == '\n' || *buf == '\r') break;
        putchar(*buf); // echo
        ++buf;
    }
    puts("\n");
    *buf = 0;
}
