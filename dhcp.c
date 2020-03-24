#include "dhcp.h"
#include "error.h"
#include "task.h"
#include "string.h"
#include "stdio.h"
#include "utils.h"

__attribute__ ((aligned (16))) uint8_t dhcp_req_buff[512];
dhcp_packet_t *const dhcp_req = (dhcp_packet_t *)dhcp_req_buff;

__attribute__ ((aligned (16))) uint8_t dhcp_buff[1024];
dhcp_packet_t *const dhcp_packet = (dhcp_packet_t *)dhcp_buff;

int dhcp_check(dhcp_packet_t *dhcp, size_t len, uint32_t xid)
{
    if (len < sizeof(dhcp_packet_t) + 4)
    {
        return -EOOB;
    }

    if (!(dhcp->op == DHCP_OP_REPLY
          && dhcp->htype == DHCP_HTYPE
          && dhcp->hlen == DHCP_HLEN
          && dhcp->xid == xid
          && dhcp->magic_cookie == DHCP_MAGIC_COOKIE
          && dhcp->options[0] == DHCP_OPTION_MESSAGE_TYPE
          && dhcp->options[1] == 0x01))
    {
        return -EBADPKT;
    }

    return 0;
}

__attribute__ ((aligned (16))) uint8_t dhcp_stack[1024];
static struct task dhcp_task;
static void dhcp_entry()
{
    if (get_ip_ready() && !ipv4_is_unspecified(get_ifinfo_ipv4()))
    {
        exit();
    }

    const uint64_t mac = get_ifinfo_mac();

    uint32_t xid = rdcycle() ^ 0xaa55ccdd;
    dhcp_req->op = DHCP_OP_REQUEST;
    dhcp_req->htype = DHCP_HTYPE;
    dhcp_req->hlen = DHCP_HLEN;
    dhcp_req->hops = 0;
    dhcp_req->xid = xid;
    dhcp_req->secs = 0;
    dhcp_req->flags = 0;
    dhcp_req->ciaddr = ipv4_unspecified();
    dhcp_req->yiaddr = ipv4_unspecified();
    dhcp_req->siaddr = ipv4_unspecified();
    dhcp_req->giaddr = ipv4_unspecified();
    memcpy(dhcp_req->chaddr, &mac, DHCP_HLEN);
    memset(dhcp_req->reserved, 0, sizeof(dhcp_req->reserved));
    dhcp_req->magic_cookie = DHCP_MAGIC_COOKIE;

    dhcp_req->options[0] = DHCP_OPTION_MESSAGE_TYPE;
    dhcp_req->options[1] = 0x01;
    dhcp_req->options[2] = DHCP_MESSAGE_TYPE_DISCOVER;
    dhcp_req->options[3] = DHCP_OPTION_END;

    printf("DHCP Client: Waiting for DHCP server... ");

    ipv6_addr_t saddr;
    uint16_t sport;
    while (1)
    {
        send_udp(DHCP_CLIENT_PORT, ipv4_compatible(ipv4_broadcast()), DHCP_SERVER_PORT,
                 dhcp_req, sizeof(dhcp_packet_t) + 4);
        size_t ret = recv_udp(DHCP_CLIENT_PORT, &saddr, &sport, dhcp_buff, sizeof(dhcp_buff),
                              ipv6_unspecified(), DHCP_SERVER_PORT, 3 * CLOCK_FREQ);
        if (ret != 0)
        {
            if (dhcp_check(dhcp_packet, ret, xid) == 0
                && dhcp_packet->options[2] == DHCP_MESSAGE_TYPE_OFFER)
            {
                break;
            }
        }
        else
        {
            printf("timed out, retrying... ");
        }
    }

    ipv4_addr_t dhcp_server_addr = extract_ipv4(saddr);
    ipv4_addr_t ip = dhcp_packet->yiaddr;
    printf("get ip ");
    print_ipv4(ip.u8);
    printf(" from ");
    print_ipv4(dhcp_server_addr.u8);
    printf("\n");
    set_ifinfo_ipv4(ip);
    set_dhcp_server_ip(ipv4_compatible(dhcp_server_addr));

    dhcp_req->options[2] = DHCP_MESSAGE_TYPE_REQUEST;
    dhcp_req->options[3] = DHCP_OPTION_REQ_ADDR;
    dhcp_req->options[4] = 0x04;
    memcpy(&dhcp_req->options[5], ip.u8, 4);
    dhcp_req->options[9] = DHCP_OPTION_END;

    printf("DHCP Client: Sending request... ");

    while (1)
    {
        send_udp(DHCP_CLIENT_PORT, ipv4_compatible(dhcp_server_addr), DHCP_SERVER_PORT,
                 dhcp_req, sizeof(dhcp_packet_t) + 10);
        ipv6_addr_t saddr;
        uint16_t sport;
        size_t ret = recv_udp(DHCP_CLIENT_PORT, &saddr, &sport, dhcp_buff, sizeof(dhcp_buff),
                              ipv4_compatible(dhcp_server_addr), DHCP_SERVER_PORT, CLOCK_FREQ / 4);
        if (ret != 0)
        {
            if (dhcp_check(dhcp_packet, ret, xid) == 0
                && dhcp_packet->options[2] == DHCP_MESSAGE_TYPE_ACK)
            {
                break;
            }
        }
        else
        {
            printf("timed out, retrying... ");
        }
    }

    ip = dhcp_packet->yiaddr;
    printf("get ip ");
    print_ipv4(ip.u8);
    printf(" from ");
    print_ipv4(dhcp_server_addr.u8);
    printf("\n");
    set_ifinfo_ipv4(ip);
    set_ip_ready(true);
    exit();
}

void init_dhcp()
{
    create_task(&dhcp_task, (uintptr_t)dhcp_stack + sizeof(dhcp_stack) + STACK_OFFSET, dhcp_entry);
}
