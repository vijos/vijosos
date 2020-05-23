#ifndef _TFTP_H_
#define _TFTP_H_

#include "stdint.h"

#define TFTP_SERVER_PORT 6969

#define TFTP_OP_RRQ 0x0100
#define TFTP_OP_WRQ 0x0200
#define TFTP_OP_DATA 0x0300
#define TFTP_OP_ACK 0x0400
#define TFTP_OP_ERROR 0x0500

#define TFTP_MODE "octet"
#define TFTP_MODE_SIZE (5 + 1)

#define TFTP_CHUNK_SIZE 512

typedef struct
{
    uint16_t opcode;
} tftp_req_packet_t;

typedef struct
{
    uint16_t opcode;
    uint16_t seq;
} tftp_packet_t;

typedef int (*tftp_read_block_t)(void *ctx, void *buff);

void init_tftp();
int tftp_get_file(char *filename, size_t filename_size, void *buff, size_t len, size_t *out_len);
int tftp_put_file(char *filename, size_t filename_size, tftp_read_block_t read_block, void *ctx);

#endif
