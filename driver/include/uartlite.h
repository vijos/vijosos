#ifndef _DRIVER_UARTLITE_H_
#define _DRIVER_UARTLITE_H_

#include "stdint.h"
#include "stdbool.h"
#include "tty.h"

extern void *UARTLITE_BASE;
#define UARTLITE_RX (*(volatile uint32_t *)((uintptr_t)UARTLITE_BASE + 0x0))
#define UARTLITE_TX (*(volatile uint32_t *)((uintptr_t)UARTLITE_BASE + 0x4))
#define UARTLITE_STAT (*(volatile uint32_t *)((uintptr_t)UARTLITE_BASE + 0x8))
#define UARTLITE_CTRL (*(volatile uint32_t *)((uintptr_t)UARTLITE_BASE + 0xc))

#define UARTLITE_TX_FIFO_FULL 8
#define UARTLITE_RX_FIFO_VALID 1

extern const tty_ops_t uartlite_ops;
void init_uartlite();

#endif
