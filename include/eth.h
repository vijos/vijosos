#ifndef _ETH_H_
#define _ETH_H_

#include "stdint.h"
#include "stdbool.h"

typedef struct
{
    void (*reset)();
    void (*begin_dma_recv)(void *data, uint32_t len);
    void (*begin_dma_send)(void *data, uint32_t len);
    bool (*poll_recv)();
    bool (*poll_send)();
    uint16_t (*last_recv_len)();
} eth_ops_t;

extern const eth_ops_t *eth_ops;

void init_eth();
void eth_reset();

void eth_begin_dma_recv(void *data, uint32_t len);
void eth_begin_dma_send(void *data, uint32_t len);

static inline bool eth_poll_recv()
{
    return eth_ops->poll_recv();
}

static inline bool eth_poll_send()
{
    return eth_ops->poll_send();
}

static inline uint16_t eth_last_recv_len()
{
    return eth_ops->last_recv_len();
}

#endif
