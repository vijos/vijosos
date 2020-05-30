#include "utils.h"
#include "stdio.h"
#include "ip.h"
#include "arch/mmu.h"

void zero_page(void *pg)
{
    uint64_t *end = (uint64_t *)((uint8_t *)pg + PGSIZE);
    uint64_t *u64 = (uint64_t *)pg;
    for (; u64 != end; u64 += 8)
    {
        u64[0] = 0;
        u64[1] = 0;
        u64[2] = 0;
        u64[3] = 0;
        u64[4] = 0;
        u64[5] = 0;
        u64[6] = 0;
        u64[7] = 0;
    }
}

void print_mem(void *data, size_t len, char delim, int newline)
{
    uint8_t *ptr = data;
    for (int i = 0; i < len; ++i)
    {
        printf("%02x", ptr[i]);
        if (newline && (i & 0xf) == 0xf)
        {
            puts("\n");
        }
        else
        {
            if (i != len - 1)
            {
                putchar(delim);
            }
        }
    }
    if (newline && (len & 0xf) != 0)
    {
        puts("\n");
    }
}

void print_mac(uint8_t *data)
{
    printf("%02x-%02x-%02x-%02x-%02x-%02x", data[0], data[1], data[2], data[3], data[4], data[5]);
}

void print_ipv4(uint8_t *data)
{
    printf("%u.%u.%u.%u", data[0], data[1], data[2], data[3]);
}

void print_ipv6(uint8_t *data)
{
    printf("%02x%02x", data[0], data[1]);
    for (int i = 1; i < 8; ++i)
    {
        printf(":%02x%02x", data[i * 2], data[i * 2 + 1]);
    }
}

void print_frame(frame_t *frame)
{
    puts("recv: ");
    print_mac(frame->src_mac); puts(" -> "); print_mac(frame->dst_mac);
    printf(", type %04x", ntohs(frame->ethertype));
    printf(", length %d\n", frame->meta.len);
}
