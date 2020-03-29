#include "tftp.h"
#include "arch/asm.h"
#include "error.h"
#include "eth.h"
#include "task.h"
#include "string.h"
#include "printf.h"
#include "elf.h"

__attribute__ ((aligned (16))) uint8_t tftp_buff[1024];
tftp_packet_t *const tftp_packet = (tftp_packet_t *)tftp_buff;

__attribute__ ((aligned (16))) uint8_t tftp_req_buff[128];
tftp_req_packet_t *const tftp_req = (tftp_req_packet_t *)tftp_req_buff;

int tftp_get_file(char *filename, size_t filename_size, void *buff, size_t len, size_t *out_len)
{
    if (sizeof(tftp_req_buff) < sizeof(tftp_req_packet_t) + filename_size + TFTP_MODE_SIZE)
    {
        return -EOOB;
    }

    uint16_t client_port;
    do
    {
        client_port = rdcycle() ^ 0x5744;
    }
    while (client_port < 1024);

    tftp_req->opcode = TFTP_OP_RRQ;
    char *req_filename = (char *)(tftp_req + 1);
    memcpy(req_filename, filename, filename_size);
    char *mode = req_filename + filename_size;
    memcpy(mode, TFTP_MODE, TFTP_MODE_SIZE);

    printf("TFTP Client: Getting %s...\n", filename);

    // Send the request and wait for the first data block.
    size_t ret;
    uint16_t server_port;
    while (1)
    {
        send_udp(client_port, server_ip, TFTP_SERVER_PORT, tftp_req,
                 sizeof(tftp_req_packet_t) + filename_size + TFTP_MODE_SIZE);
        ipv6_addr_t saddr;
        ret = recv_udp(client_port, &saddr, &server_port, tftp_buff, sizeof(tftp_buff),
                       server_ip, 0, CLOCK_FREQ / 4);
        if (ret != 0) break;
    }

    if (ret < sizeof(tftp_packet_t))
    {
        printf("TFTP Client: UDP datagram is too small.\n");
        return -EBADPKT;
    }

    if (tftp_packet->opcode == TFTP_OP_ERROR)
    {
        printf("TFTP Client: Error %d: %s\n",
               ntohs(tftp_packet->seq), (uint8_t *)(tftp_packet + 1));
        return -EBADPKT;
    }
    else if (tftp_packet->opcode != TFTP_OP_DATA)
    {
        printf("TFTP Client: Unknown Opcode %d\n", ntohs(tftp_packet->opcode));
        return -EBADPKT;
    }

    if (tftp_packet->seq != htons(1))
    {
        printf("TFTP Client: Not First Block???\n");
        return -EBADPKT;
    }

    tftp_packet->opcode = TFTP_OP_ACK;
    send_udp(client_port, server_ip, server_port, tftp_packet, sizeof(tftp_packet_t));

    size_t data_len = ret - sizeof(tftp_packet_t);
    size_t total_len = data_len;

    uint8_t *u8buff = (uint8_t *)buff;
    memcpy(u8buff, (void *)(tftp_packet + 1), data_len);
    u8buff += data_len;

    if (data_len < TFTP_CHUNK_SIZE)
    {
        printf("TFTP Client: Done, received %lu bytes.\n", total_len);
        if (out_len) *out_len = total_len;
        return 0;
    }

    printf("TFTP Client: Received %lu bytes.", total_len);

    uint16_t window = 2;
    while (1)
    {
        ipv6_addr_t saddr;
        uint16_t sport;
        ret = recv_udp(client_port, &saddr, &sport, tftp_buff, sizeof(tftp_buff),
                       server_ip, server_port, 30 * CLOCK_FREQ);
        if (ret == 0)
        {
            printf("TFTP Client: Timed out!\n");
            return -EBADPKT;
        }

        if (ret < sizeof(tftp_packet_t))
        {
            printf("TFTP Client: UDP datagram is too small.\n");
            return -EBADPKT;
        }

        if (tftp_packet->opcode == TFTP_OP_ERROR)
        {
            printf("TFTP Client: Error %d: %s\n",
                   ntohs(tftp_packet->seq), (uint8_t *)(tftp_packet + 1));
            return -EBADPKT;
        }
        else if (tftp_packet->opcode != TFTP_OP_DATA)
        {
            printf("TFTP Client: Unknown Opcode %d\n", ntohs(tftp_packet->opcode));
            return -EBADPKT;
        }

        tftp_packet->opcode = TFTP_OP_ACK;
        send_udp(client_port, server_ip, server_port, tftp_packet, sizeof(tftp_packet_t));

        if (ntohs(tftp_packet->seq) == window)
        {
            data_len = ret - sizeof(tftp_packet_t);
            total_len += data_len;
            memcpy(u8buff, (void *)(tftp_packet + 1), data_len);
            u8buff += data_len;

            if ((total_len & 0x7ffff) == 0)
            {
                printf("\rTFTP Client: Received %lu bytes.          ", total_len);
            }

            if (data_len < TFTP_CHUNK_SIZE)
            {
                printf("\nTFTP Client: Done, received %lu bytes.\n", total_len);
                if (out_len) *out_len = total_len;
                return 0;
            }

            ++window;
        }
        else
        {
            printf("TFTP Client: Warning: Block %d is not in the window.\n",
                   ntohs(tftp_packet->seq));
        }
    }
}

void init_tftp()
{

}
