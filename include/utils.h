#ifndef _UTILS_H_
#define _UTILS_H_

#include "stdint.h"
#include "ip.h"

void zero_page(void *pg);
void print_mem(void *data, size_t len, char delim, int newline);
void print_mac(uint8_t *data);
void print_ipv4(uint8_t *data);
void print_ipv6(uint8_t *data);
void print_frame(frame_t *frame);

static inline size_t bits_find_zero(uint64_t b)
{
    b = ~b;
    b = b & -b;  // to one-hot
    size_t i = 0;
    if (b & 0xffffffff00000000ul) i += 32;
    if (b & 0xffff0000ffff0000ul) i += 16;
    if (b & 0xff00ff00ff00ff00ul) i += 8;
    if (b & 0xf0f0f0f0f0f0f0f0ul) i += 4;
    if (b & 0xccccccccccccccccul) i += 2;
    if (b & 0xaaaaaaaaaaaaaaaaul) i += 1;
    return i;
}

#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))

#endif
