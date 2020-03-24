#ifndef _DRIVER_UART16550_H_
#define _DRIVER_UART16550_H_

#include "stdint.h"
#include "stdbool.h"
#include "tty.h"

extern volatile uint8_t *UART16550_BASE;

extern const tty_ops_t uart16550_ops;
void init_uart16550();

#endif
