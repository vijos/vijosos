#include "utils.h"
#include "stdio.h"
#include "ip.h"

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
