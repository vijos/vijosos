#include "driver/axiethernet.h"
#include "eth.h"
#include "arch/asm.h"
#include "arch/mm.h"
#include "stdio.h"

void *DMA_BASE;
void *ETH_BASE;

static bool send_not_begin = true;

__attribute__ ((aligned (0x40))) static dma_desc_t rx_desc;
__attribute__ ((aligned (0x40))) static dma_desc_t tx_desc;

static void reset();

void init_axiethernet()
{
    eth_ops = &axiethernet_ops;

    send_not_begin = true;

    reset();

    puts("Initializing ethernet... ");

    rx_desc.next = v2p(&rx_desc);
    tx_desc.next = v2p(&tx_desc);

    uintptr_t ptr = v2p(&rx_desc);
    DMA_S2MM_CURDESC = (uint32_t)(ptr & 0xffffffff);
    DMA_S2MM_CURDESC_MSB = (uint32_t)((ptr >> 32) & 0xffffffff);
    DMA_S2MM_DMACR = 0;
    DMA_S2MM_DMACR |= DMA_DMACR_RS;
    while (DMA_S2MM_DMASR & DMA_DMASR_HALTED);

    ptr = v2p(&tx_desc);
    DMA_MM2S_CURDESC = (uint32_t)(ptr & 0xffffffff);
    DMA_MM2S_CURDESC_MSB = (uint32_t)((ptr >> 32) & 0xffffffff);
    DMA_MM2S_DMACR = 0;
    DMA_MM2S_DMACR |= DMA_DMACR_RS;
    while (DMA_MM2S_DMASR & DMA_DMASR_HALTED);

    puts("OK\n");
}

static void reset()
{
    puts("Resetting ethernet... ");

    ETH_RCW1R |= ETH_RCW1R_RX;
    ETH_TCR |= ETH_TCR_TX;

    ETH_RCW1R |= ETH_RCW1R_RST;
    while (ETH_RCW1R & ETH_RCW1R_RST);
    ETH_TCR |= ETH_TCR_RST;
    while (ETH_TCR & ETH_TCR_RST);

    DMA_S2MM_DMACR = 0;
    DMA_S2MM_DMACR |= DMA_DMACR_RESET;
    while (DMA_S2MM_DMACR & DMA_DMACR_RESET);
    DMA_MM2S_DMACR = 0;
    DMA_MM2S_DMACR |= DMA_DMACR_RESET;
    while (DMA_MM2S_DMACR & DMA_DMACR_RESET);

    puts("OK\n");
}

static void begin_dma_recv(void *data, uint32_t len)
{
    rx_desc.buff = v2p(data);
    rx_desc.control = len & DMA_DESC_CONTROL_LENGTH_MASK;
    rx_desc.status = 0;

    uintptr_t ptr = v2p(&rx_desc);
    DMA_S2MM_TAILDESC = (uint32_t)(ptr & 0xffffffff);
    fence();
    DMA_S2MM_TAILDESC_MSB = (uint32_t)((ptr >> 32) & 0xffffffff);
}

static void begin_dma_send(void *data, uint32_t len)
{
    send_not_begin = false;
    tx_desc.buff = v2p(data);
    tx_desc.control = (len & DMA_DESC_CONTROL_LENGTH_MASK)
                      | DMA_DESC_CONTROL_TXSOF | DMA_DESC_CONTROL_TXEOF;
    tx_desc.status = 0;

    uintptr_t ptr = v2p(&tx_desc);
    DMA_MM2S_TAILDESC = (uint32_t)(ptr & 0xffffffff);
    fence();
    DMA_MM2S_TAILDESC_MSB = (uint32_t)((ptr >> 32) & 0xffffffff);
}

static bool poll_recv()
{
    return DMA_S2MM_DMASR & DMA_DMASR_IDLE;
}

static bool poll_send()
{
    return send_not_begin || (DMA_MM2S_DMASR & DMA_DMASR_IDLE);
}

static uint16_t last_recv_len()
{
    return rx_desc.status & DMA_DESC_STATUS_LENGTH_MASK;
}

const eth_ops_t axiethernet_ops =
{
    .reset = reset,
    .begin_dma_recv = begin_dma_recv,
    .begin_dma_send = begin_dma_send,
    .poll_recv = poll_recv,
    .poll_send = poll_send,
    .last_recv_len = last_recv_len
};
