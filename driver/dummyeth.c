#include "driver/dummyeth.h"
#include "eth.h"
#include "stdio.h"

void init_dummyeth()
{
    eth_ops = &dummyeth_ops;
    puts("Using dummy ethernet...\n");
}

static void reset()
{

}

static void sendrecv(void *data, uint32_t len)
{

}

static bool poll_sendrecv()
{
    return false;
}

static uint16_t last_recv_len()
{
    return 0;
}

const eth_ops_t dummyeth_ops =
{
    .reset = reset,
    .begin_dma_recv = sendrecv,
    .begin_dma_send = sendrecv,
    .poll_recv = poll_sendrecv,
    .poll_send = poll_sendrecv,
    .last_recv_len = last_recv_len
};
