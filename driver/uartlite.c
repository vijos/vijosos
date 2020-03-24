#include "driver/uartlite.h"
#include "tty.h"

void *UARTLITE_BASE;

void init_uartlite()
{
    tty_ops = &uartlite_ops;
}

static bool poll_recv()
{
    return UARTLITE_STAT & UARTLITE_RX_FIFO_VALID;
}

static bool poll_send()
{
    return !(UARTLITE_STAT & UARTLITE_TX_FIFO_FULL);
}

static void send(uint8_t c)
{
    if (c == '\n')
    {
        send('\r');
    }
    while (!poll_send());
    UARTLITE_TX = c;
}

static uint8_t recv()
{
    while (!poll_recv());
    return UARTLITE_RX;
}

const tty_ops_t uartlite_ops =
{
    .putchar = send,
    .getchar = recv,
    .poll_recv = poll_recv,
    .poll_send = poll_send
};
