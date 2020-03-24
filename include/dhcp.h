#ifndef _DHCP_H_
#define _DHCP_H_

#include "stdint.h"
#include "ip.h"

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

#define DHCP_OP_REQUEST 1
#define DHCP_OP_REPLY 2
#define DHCP_HTYPE 1
#define DHCP_HLEN 6
#define DHCP_MAGIC_COOKIE 0x63538263

#define DHCP_OPTION_MESSAGE_TYPE 0x35
#define DHCP_OPTION_REQ_ADDR 0x32
#define DHCP_OPTION_END 0xff

#define DHCP_MESSAGE_TYPE_DISCOVER 0x01
#define DHCP_MESSAGE_TYPE_OFFER 0x02
#define DHCP_MESSAGE_TYPE_REQUEST 0x03
#define DHCP_MESSAGE_TYPE_ACK 0x05

typedef struct
{
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    ipv4_addr_t ciaddr;
    ipv4_addr_t yiaddr;
    ipv4_addr_t siaddr;
    ipv4_addr_t giaddr;
    uint8_t chaddr[DHCP_HLEN];
    uint8_t reserved[(16 - DHCP_HLEN) + 192];
    uint32_t magic_cookie;
    uint8_t options[];
} dhcp_packet_t;

void init_dhcp();

#endif
