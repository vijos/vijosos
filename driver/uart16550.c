#include "driver/uart16550.h"
#include "tty.h"

volatile uint8_t *UART16550_BASE;
// some devices require a shifted register index
// (e.g. 32 bit registers instead of 8 bit registers)
static uint32_t uart16550_reg_shift;
static uint32_t uart16550_clock = 0x384000;   // a "common" base clock

#define UART_REG_QUEUE     0    // rx/tx fifo data
#define UART_REG_DLL       0    // divisor latch (LSB)
#define UART_REG_IER       1    // interrupt enable register
#define UART_REG_DLM       1    // divisor latch (MSB) 
#define UART_REG_FCR       2    // fifo control register
#define UART_REG_LCR       3    // line control register
#define UART_REG_MCR       4    // modem control register
#define UART_REG_LSR       5    // line status register
#define UART_REG_MSR       6    // modem status register
#define UART_REG_SCR       7    // scratch register
#define UART_REG_STATUS_RX 0x01
#define UART_REG_STATUS_TX 0x20

// We cannot use the word DEFAULT for a parameter that cannot be overridden due to -Werror
#ifndef UART_DEFAULT_BAUD
#define UART_DEFAULT_BAUD  38400
#endif

void init_uart16550()
{
    tty_ops = &uart16550_ops;

    uint32_t divisor = uart16550_clock / (16 * UART_DEFAULT_BAUD);

    uart16550_reg_shift = 0;
    // http://wiki.osdev.org/Serial_Ports
    UART16550_BASE[UART_REG_IER << uart16550_reg_shift] = 0x00;                // Disable all interrupts
    UART16550_BASE[UART_REG_LCR << uart16550_reg_shift] = 0x80;                // Enable DLAB (set baud rate divisor)
    UART16550_BASE[UART_REG_DLL << uart16550_reg_shift] = (uint8_t)divisor;    // Set divisor (lo byte)
    UART16550_BASE[UART_REG_DLM << uart16550_reg_shift] = (uint8_t)(divisor >> 8);     //     (hi byte)
    UART16550_BASE[UART_REG_LCR << uart16550_reg_shift] = 0x03;                // 8 bits, no parity, one stop bit
    UART16550_BASE[UART_REG_FCR << uart16550_reg_shift] = 0xC7;                // Enable FIFO, clear them, with 14-byte threshold
}

static bool poll_recv()
{
    return UART16550_BASE[UART_REG_LSR << uart16550_reg_shift] & UART_REG_STATUS_RX;
}

static bool poll_send()
{
    return UART16550_BASE[UART_REG_LSR << uart16550_reg_shift] & UART_REG_STATUS_TX;
}

static void send(uint8_t c)
{
    if (c == '\n')
    {
        send('\r');
    }
    while (!poll_send());
    UART16550_BASE[UART_REG_QUEUE << uart16550_reg_shift] = c;
}

static uint8_t recv()
{
    while (!poll_recv());
    return UART16550_BASE[UART_REG_QUEUE << uart16550_reg_shift];
}

const tty_ops_t uart16550_ops =
{
    .putchar = send,
    .getchar = recv,
    .poll_recv = poll_recv,
    .poll_send = poll_send
};
