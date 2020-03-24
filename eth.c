#include "eth.h"
#include "arch/asm.h"
#include "stdio.h"

const eth_ops_t *eth_ops;

void init_eth()
{
}

void eth_reset()
{
    puts("Resetting ethernet... ");
    eth_ops->reset();
    puts("OK\n");
}

void eth_begin_dma_recv(void *data, uint32_t len)
{
    eth_ops->begin_dma_recv(data, len);
}

void eth_begin_dma_send(void *data, uint32_t len)
{
    eth_ops->begin_dma_send(data, len);
}
