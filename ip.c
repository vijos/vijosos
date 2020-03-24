#include "ip.h"
#include "error.h"
#include "eth.h"
#include "task.h"
#include "string.h"

const uint64_t MAC_BROADCAST = 0xffffffffffff;

interface_info_t ifinfo;
bool ip_ready = false;

ipv6_addr_t server_ip;
uint64_t server_mac = MAC_BROADCAST;
uint64_t server_mac_atime = 0;

ipv6_addr_t dhcp_server_ip;
uint64_t dhcp_server_mac = MAC_BROADCAST;
uint64_t dhcp_server_mac_atime = 0;

__attribute__ ((aligned (16))) static uint8_t dma_buff[4096];
__attribute__ ((aligned (16))) static uint8_t iptask_stack[1024];
static struct task iptask_task;
void iptask_entry()
{
    frame_t *const frame = (frame_t *)(dma_buff + ALIGN_OFFSET);
    const uint64_t len = sizeof(dma_buff) - ALIGN_OFFSET;

    while (true)
    {
        eth_begin_dma_recv((uint8_t *)frame + sizeof(frame_meta_t), len - sizeof(frame_meta_t));
        wait(WAIT_ETH_RECV);
        frame->meta.len = eth_last_recv_len();
        handle_frame(frame);
    }
}

void init_ip()
{
    server_mac = MAC_BROADCAST;
    dhcp_server_mac = MAC_BROADCAST;

    ifinfo.mtu = 1500;
    ifinfo.mac = 0x315f32445754;  // 54:57:44:32:5f:31
    ifinfo.ipv4 = ipv4_unspecified();
    /* // To use static IPv4 address:
    ifinfo.ipv4 = make_ipv4(0x8001a8c0);
    ip_ready = true;
    */
    ifinfo.ipv6 = ipv6_unspecified();
    server_ip = ipv4_compatible(make_ipv4(0x3701a8c0));  // 192.168.1.55

    create_task(&iptask_task,
                (uintptr_t)iptask_stack + sizeof(iptask_stack) + STACK_OFFSET,
                iptask_entry);
}

uint64_t get_ifinfo_mac()
{
    return ifinfo.mac;
}

ipv4_addr_t get_ifinfo_ipv4()
{
    return ifinfo.ipv4;
}

void set_ifinfo_ipv4(ipv4_addr_t ipv4)
{
    ifinfo.ipv4 = ipv4;
}

bool get_ip_ready()
{
    return ip_ready;
}

void set_ip_ready(bool ready)
{
    ip_ready = ready;
}

void set_dhcp_server_ip(ipv6_addr_t ip)
{
    dhcp_server_ip = ip;
    dhcp_server_mac = MAC_BROADCAST;
}

int ip_query_route(ipv6_addr_t addr, ipv6_addr_t *nexthop)
{
    // TODO: route table
    *nexthop = addr;
    return 0;
}

int ip_query_mac(ipv6_addr_t addr, uint64_t *mac)
{
    // TODO: mac cache
    if (ipv6_eq(addr, server_ip))
    {
        if (server_mac == MAC_BROADCAST)
        {
            return -ENOFOUND;
        }
        *mac = server_mac;
        return 0;
    }
    if (ipv6_eq(addr, dhcp_server_ip))
    {
        if (dhcp_server_mac == MAC_BROADCAST)
        {
            return -ENOFOUND;
        }
        *mac = dhcp_server_mac;
        return 0;
    }
    if (ipv6_eq(addr, ipv4_compatible(ipv4_broadcast())))
    {
        *mac = MAC_BROADCAST;
        return 0;
    }
    return -ENOFOUND;
}

void ip_update_mac(ipv6_addr_t addr, uint64_t mac)
{
    if (ipv6_eq(addr, server_ip))
    {
        server_mac = mac;
        server_mac_atime = rdcycle();
    }
    if (ipv6_eq(addr, dhcp_server_ip))
    {
        dhcp_server_mac = mac;
        dhcp_server_mac_atime = rdcycle();
    }
}

void ip_update_mac_atime(ipv6_addr_t addr)
{
    if (ipv6_eq(addr, server_ip))
    {
        server_mac_atime = rdcycle();
    }
    if (ipv6_eq(addr, dhcp_server_ip))
    {
        dhcp_server_mac_atime = rdcycle();
    }
}

int ip_send(frame_t *frame, ipv6_addr_t nexthop)
{
    int ret;
    uint64_t mac;
    if ((ret = ip_query_mac(nexthop, &mac)) < 0)
    {
        /*
            TODO: save this packet, and send an ARP request or a NDP solicitation.
        */
        bool is_v4 = is_ipv4(nexthop);
        int ret;
        if (is_v4)
        {
            ret = arp_make_request(frame, 4096 /* FIXME */, extract_ipv4(nexthop), -1);
            if (ret < 0) return ret;
        }
        else
        {
            ret = ip_make_ndp(frame, 4096 /* FIXME */, nexthop, -1, false, false, 0);
            if (ret < 0) return ret;
        }
        eth_begin_send_frame(frame);
        wait(WAIT_ETH_SEND);
        return 0;
    }
    return ip_send_with_mac(frame, mac);
}

int ip_send_with_mac(frame_t *frame, uint64_t mac)
{
    memcpy(frame->src_mac, &ifinfo.mac, sizeof(frame->src_mac));
    memcpy(frame->dst_mac, &mac, sizeof(frame->dst_mac));
    eth_begin_send_frame(frame);
    wait(WAIT_ETH_SEND);
    return 0;
}

uint16_t ipv6_l4_checksum(ipv6_header_t *ipv6)
{
    uint32_t checksum = 0;
    checksum += ip_checksum_partial(ipv6->src.u8, sizeof(ipv6->src.u8));
    checksum += ip_checksum_partial(ipv6->dst.u8, sizeof(ipv6->dst.u8));
    checksum += ipv6->payload_len;
    checksum += (uint16_t)ipv6->next_header << 8;
    checksum += ip_checksum_partial(ipv6 + 1, ntohs(ipv6->payload_len));
    return ip_checksum_final(checksum);
}

uint16_t ipv4_l4_checksum(ipv4_header_t *ipv4, uint16_t payload_len)
{
    uint32_t checksum = 0;
    checksum += ip_checksum_partial(ipv4->src.u8, sizeof(ipv4->src.u8));
    checksum += ip_checksum_partial(ipv4->dst.u8, sizeof(ipv4->dst.u8));
    checksum += htons(payload_len);
    checksum += (uint16_t)ipv4->next_header << 8;
    checksum += ip_checksum_partial(ipv4 + 1, payload_len);
    return ip_checksum_final(checksum);
}

int arp_make_request(frame_t *frame, size_t buflen, ipv4_addr_t ip, uint64_t known_mac)
{
    if (buflen < sizeof(frame_t) + sizeof(arp_packet_t)) return -EOOB;
    arp_packet_t *arp = (arp_packet_t *)frame->payload;

    frame->meta.len = sizeof(eth_header_t) + sizeof(arp_packet_t);

    memcpy(frame->dst_mac, &known_mac, 6);
    memcpy(frame->src_mac, &ifinfo.mac, 6);
    frame->ethertype = ETH_TYPE_ARP;

    arp->htype = ARP_HTYPE_ETHERNET;
    arp->ptype = ARP_PTYPE_IPV4;
    arp->hlen = 6;
    arp->plen = 4;
    arp->op = ARP_OP_REQUEST;
    memcpy(arp->mac_saddr, &ifinfo.mac, 6);
    memcpy(arp->ip_saddr, &ifinfo.ipv4, 4);
    memcpy(arp->mac_daddr, &known_mac, 6);
    memcpy(arp->ip_daddr, &ip, 4);
    return 0;
}

int handle_arp(frame_t *frame)
{
    if (frame->meta.len < sizeof(eth_header_t) + sizeof(arp_packet_t)) return -EOOB;
    arp_packet_t *arp = (arp_packet_t *)frame->payload;

    if (!(arp->htype == ARP_HTYPE_ETHERNET && arp->ptype == ARP_PTYPE_IPV4
          && arp->hlen == 6 && arp->plen == 4)) return -EBADPKT;

    if (arp->op != ARP_OP_REQUEST && arp->op != ARP_OP_REPLY) return -EBADPKT;

    uint64_t src_mac = 0;
    memcpy(&src_mac, arp->mac_saddr, 6);
    ipv4_addr_t src_ip;
    memcpy(&src_ip, arp->ip_saddr, 4);

    bool is_to_me = memcmp(arp->ip_daddr, &ifinfo.ipv4, 4) == 0;

    // update ARP table
    if (!ipv4_is_unspecified(src_ip))
    {
        ip_update_mac(ipv4_compatible(src_ip), src_mac);
    }

    // reply ARP request
    if (arp->op == ARP_OP_REQUEST && is_to_me)
    {
        memcpy(frame->dst_mac, &src_mac, 6);
        memcpy(frame->src_mac, &ifinfo.mac, 6);
        frame->ethertype = ETH_TYPE_ARP;

        arp->op = ARP_OP_REPLY;
        memcpy(arp->mac_saddr, &ifinfo.mac, 6);
        memcpy(arp->ip_saddr, &ifinfo.ipv4, 4);
        memcpy(arp->mac_daddr, &src_mac, 6);
        memcpy(arp->ip_daddr, &src_ip, 4);

        frame->meta.len = sizeof(eth_header_t) + sizeof(arp_packet_t);

        eth_begin_send_frame(frame);
        wait(WAIT_ETH_SEND);
    }

    return 0;
}

int ip_make_mld_report(frame_t *frame, size_t buflen, ipv6_addr_t maddr)
{
    if (buflen < sizeof(frame_t) + sizeof(ipv6_header_t)
                 + sizeof(icmpv6_header_t) + sizeof(mld_packet_t))
    {
        return -EOOB;
    }

    uint64_t dst_mac = ipv6_multicast_mac(maddr);

    frame->meta.len = sizeof(eth_header_t) + sizeof(ipv6_header_t)
                      + sizeof(icmpv6_header_t) + sizeof(mld_packet_t);
    memcpy(frame->dst_mac, &dst_mac, 6);
    memcpy(frame->src_mac, &ifinfo.mac, 6);
    frame->ethertype = ETH_TYPE_IPV6;

    ipv6_header_t *ipv6 = (ipv6_header_t *)frame->payload;
    ipv6->version_flow = IPV6_VERSION;
    ipv6->payload_len = htons(sizeof(icmpv6_header_t) + sizeof(mld_packet_t));
    ipv6->next_header = IPV6_NEXT_HEADER_ICMP;
    ipv6->hop_limit = MLD_HOP_LIMIT;
    ipv6->src = ipv6_from_mac(ifinfo.mac);
    ipv6->dst = maddr;

    icmpv6_header_t *icmp = (icmpv6_header_t *)(ipv6 + 1);
    icmp->type_code = ICMPV6_TYPE_CODE_MLD_REPORT;
    icmp->checksum = 0;
    icmp->max_response_delay = 0;
    icmp->reserved1 = 0;

    mld_packet_t *mld = (mld_packet_t *)(icmp + 1);
    mld->maddr = maddr;

    icmp->checksum = ipv6_l4_checksum(ipv6);
    return 0;
}

int ip_make_ndp(frame_t *frame, size_t buflen,
                ipv6_addr_t addr, uint64_t mac,
                bool adv, bool unicast, uint32_t flags)
{
    if (buflen < sizeof(frame_t) + sizeof(ipv6_header_t)
                 + sizeof(icmpv6_header_t) + sizeof(ndp_packet_t) + sizeof(ndp_option_t))
    {
        return -EOOB;
    }

    ipv6_addr_t dst_ip;
    uint64_t dst_mac;
    if (unicast)
    {
        dst_ip = addr;
        dst_mac = mac;
    }
    else
    {
        if (!adv)
        {
            dst_ip = ipv6_solicited_node(addr);
        }
        else
        {
            dst_ip = ipv6_all_nodes();
        }
        dst_mac = ipv6_multicast_mac(dst_ip);
    }

    frame->meta.len = sizeof(eth_header_t) + sizeof(ipv6_header_t)
                      + sizeof(icmpv6_header_t) + sizeof(ndp_packet_t) + sizeof(ndp_option_t);
    memcpy(frame->dst_mac, &dst_mac, 6);
    memcpy(frame->src_mac, &ifinfo.mac, 6);
    frame->ethertype = ETH_TYPE_IPV6;

    ipv6_header_t *ipv6 = (ipv6_header_t *)frame->payload;
    ipv6->version_flow = IPV6_VERSION;
    ipv6->payload_len = htons(sizeof(icmpv6_header_t) + sizeof(ndp_packet_t) + sizeof(ndp_option_t));
    ipv6->next_header = IPV6_NEXT_HEADER_ICMP;
    ipv6->hop_limit = NDP_HOP_LIMIT;
    ipv6->src = ifinfo.ipv6;
    ipv6->dst = dst_ip;

    icmpv6_header_t *icmp = (icmpv6_header_t *)(ipv6 + 1);
    icmp->type_code = !adv ? ICMPV6_TYPE_CODE_NS : ICMPV6_TYPE_CODE_NA;
    icmp->checksum = 0;
    icmp->flags = adv ? (ICMPV6_FLAGS_R | flags) : 0;

    ndp_packet_t *ndp = (ndp_packet_t *)(icmp + 1);
    if (!adv)
    {
        ndp->addr = addr;
    }
    else
    {
        ndp->addr = ifinfo.ipv6;
    }

    ndp_option_t *option = (ndp_option_t *)(ndp + 1);
    option->type = !adv ? ICMPV6_OPTION_TYPE_SOURCE : ICMPV6_OPTION_TYPE_TARGET;
    option->len = 1;
    memcpy(option->mac_addr, &ifinfo.mac, 6);

    icmp->checksum = ipv6_l4_checksum(ipv6);
    return 0;
}

void ip_send_init_packets(frame_t *frame, size_t buflen)
{
    /*
        System starts.
        Send a gratuitous ARP packet.
        Join ff02::1 multicast group, since this is a node.
        Join ff02::2 multicast group, since this is also a router.
        Join Solicited-Node multicast group.
        Send unsolicited NDP node advertisement packet.
        Ref: https://tools.ietf.org/html/rfc4291#page-16
    */

    if (arp_make_request(frame, buflen, ifinfo.ipv4, -1) == 0)
    {
        eth_begin_send_frame(frame);
        wait(WAIT_ETH_SEND);
    }
    for (int i = 0; i < 4; ++i)
    {
        if (ip_make_mld_report(frame, buflen, ifinfo.ipv6_multicast[i]) == 0)
        {
            eth_begin_send_frame(frame);
            wait(WAIT_ETH_SEND);
        }
    }
    if (ip_make_ndp(frame, buflen, make_ipv6(0, 0), -1, true, false, ICMPV6_FLAGS_O) == 0)
    {
        eth_begin_send_frame(frame);
        wait(WAIT_ETH_SEND);
    }
}

int handle_frame(frame_t *frame)
{
    bool is_to_me = (frame->dst_mac[0] & 0x01)
                    || (memcmp(frame->dst_mac, &ifinfo.mac, 6) == 0);
    if (!is_to_me) return 0;

    switch (frame->ethertype)
    {
    case ETH_TYPE_ARP:
        return handle_arp(frame);
        break;
    case ETH_TYPE_IPV4:
        return handle_ipv4(frame);
        break;
    case ETH_TYPE_IPV6:
        return handle_ipv6(frame);
        break;
    default:
        return -ENOIMPL;
        break;
    }
}

bool ipv4_is_to_me(ipv4_addr_t dst)
{
    return ipv4_eq(dst, ifinfo.ipv4) || ipv4_is_unspecified(ifinfo.ipv4);
}

int do_icmpv4_echo(frame_t *frame)
{
    // special path to send icmpv4 echo reply

    ipv4_header_t *ipv4 = (ipv4_header_t *)frame->payload;
    icmpv4_header_t *icmp = (icmpv4_header_t *)(ipv4 + 1);

    if (icmp->code != 0) return -EBADPKT;

    frame->meta.len = sizeof(eth_header_t) + ntohs(ipv4->total_len);

    memcpy(frame->dst_mac, frame->src_mac, 6);
    memcpy(frame->src_mac, &ifinfo.mac, 6);

    ipv4->hop_limit = DEFAULT_HOP_LIMIT;

    ipv4_addr_t old_dst = ipv4->dst;
    ipv4->dst = ipv4->src;
    if (ipv4_is_multicast(old_dst))
    {
        ipv4->src = ifinfo.ipv4;
    }
    else
    {
        ipv4->src = old_dst;
    }

    ipv4->checksum = 0;
    ipv4->checksum = ip_checksum(ipv4, sizeof(ipv4_header_t));

    icmp->type = ICMPV4_TYPE_ECHO_REPLY;

    uint32_t checksum = ~icmp->checksum & 0xffff;
    checksum += (~ICMPV4_TYPE_ECHO_REQUEST & 0xffff) + ICMPV4_TYPE_ECHO_REPLY;
    icmp->checksum = ip_checksum_final(checksum);

    eth_begin_send_frame(frame);
    wait(WAIT_ETH_SEND);
    return 0;
}

int handle_icmpv4(frame_t *frame)
{
    if (frame->meta.len < sizeof(eth_header_t) + sizeof(ipv4_header_t)
                          + sizeof(icmpv4_header_t)) return -EOOB;
    ipv4_header_t *ipv4 = (ipv4_header_t *)frame->payload;
    icmpv4_header_t *icmp = (icmpv4_header_t *)(ipv4 + 1);

    uint16_t checksum = ip_checksum(icmp, ntohs(ipv4->total_len) - sizeof(ipv4_header_t));
    if (checksum != 0) return -EBADPKT;

    switch (icmp->type)
    {
    case ICMPV4_TYPE_ECHO_REQUEST:
        return do_icmpv4_echo(frame);
        break;
    default:
        return -ENOIMPL;
        break;
    }
}

int handle_ipv4(frame_t *frame)
{
    if (frame->meta.len < sizeof(eth_header_t) + sizeof(ipv4_header_t)) return -EOOB;

    ipv4_header_t *ipv4 = (ipv4_header_t *)frame->payload;
    if (ip_version(ipv4) != 4) return -EBADPKT;
    if ((ipv4->version_header_len & 0xf) != 0x5) return -ENOIMPL;

    uint16_t checksum = ip_checksum(ipv4, sizeof(ipv4_header_t));
    if (checksum != 0) return -EBADPKT;

    if (frame->meta.len < sizeof(eth_header_t) + ntohs(ipv4->total_len)) return -EOOB;
    if (ntohs(ipv4->total_len) < sizeof(ipv4_header_t)) return -EBADPKT;
    frame->meta.len = sizeof(eth_header_t) + ntohs(ipv4->total_len);

    bool is_to_me = ipv4_is_to_me(ipv4->dst);
    if (!is_to_me) return 0;

    // Only receive DHCP packets before IP address gets ready.
    if (!ip_ready && ipv4->next_header != IP_NEXT_HEADER_UDP) return 0;

    switch (ipv4->next_header)
    {
    case IPV4_NEXT_HEADER_ICMP:
        return handle_icmpv4(frame);
        break;
    case IP_NEXT_HEADER_UDP:
    {
        uint64_t payload_len = ntohs(ipv4->total_len) - sizeof(ipv4_header_t);
        if (payload_len < sizeof(udp_header_t)) return -EOOB;
        udp_header_t *udp = (udp_header_t *)(ipv4 + 1);
        if (udp->checksum)
        {
            checksum = ipv4_l4_checksum(ipv4, payload_len);
            if (checksum != 0) return -EBADPKT;
        }
        return handle_udp(ipv4_compatible(ipv4->src), (udp_header_t *)(ipv4 + 1),
                          frame->meta.len - (sizeof(eth_header_t) + sizeof(ipv4_header_t)));
        break;
    }
    default:
        return -ENOIMPL;
        break;
    }
}

bool ipv6_is_to_me(ipv6_addr_t dst)
{
    if (ipv6_eq(dst, ifinfo.ipv6)) return true;
    for (int i = 0; i < 4; ++i)
    {
        if (ipv6_eq(dst, ifinfo.ipv6_multicast[i])) return true;
    }
    return false;
}

int handle_ndp(frame_t *frame)
{
    if (frame->meta.len < sizeof(eth_header_t) + sizeof(ipv6_header_t)
                          + sizeof(icmpv6_header_t) + sizeof(ndp_packet_t)) return -EOOB;
    ipv6_header_t *ipv6 = (ipv6_header_t *)frame->payload;
    icmpv6_header_t *icmp = (icmpv6_header_t *)(ipv6 + 1);
    ndp_packet_t *ndp = (ndp_packet_t *)(icmp + 1);

    if (ipv6->hop_limit != NDP_HOP_LIMIT) return -EBADPKT;
    if (icmp->code != 0) return -EBADPKT;

    bool adv = icmp->type == ICMPV6_TYPE_NA;
    bool has_option = frame->meta.len >= sizeof(eth_header_t) + sizeof(ipv6_header_t)
                      + sizeof(icmpv6_header_t) + sizeof(ndp_packet_t) + sizeof(ndp_option_t);
    uint64_t option_mac = 0;
    if (has_option)
    {
        ndp_option_t *option = (ndp_option_t *)(ndp + 1);
        has_option = option->len == 1 && option->type == (!adv ? ICMPV6_OPTION_TYPE_SOURCE
                                                               : ICMPV6_OPTION_TYPE_TARGET);
        if (has_option)
        {
            memcpy(&option_mac, option->mac_addr, 6);
        }
    }

    uint64_t src_mac = 0;
    memcpy(&src_mac, frame->src_mac, 6);
    ipv6_addr_t src_ip = ipv6->src;
    ipv6_addr_t target_addr = ndp->addr;

    // TODO: what if has_option && src_mac != option_mac???
    // TODO: what if adv && src_ip != target_addr???

    if (!adv)
    {
        bool is_to_me = ipv6_eq(target_addr, ifinfo.ipv6);
        if (has_option && !ipv6_is_unspecified(src_ip))
        {
            ip_update_mac(src_ip, option_mac);
        }

        if (is_to_me)
        {
            int ret;
            if (!ipv6_is_unspecified(src_ip))
            {
                // FIXME: according to RFC 4861, we should either use the MAC address in ndp option,
                // or do Neighbor Discovery to figure out the MAC address of src_ip.
                // Don't use src_mac!
                ret = ip_make_ndp(frame, 4096, src_ip, src_mac,
                                  true, true, ICMPV6_FLAGS_S | ICMPV6_FLAGS_O);
            }
            else
            {
                ret = ip_make_ndp(frame, 4096, make_ipv6(0, 0), -1, true, false, ICMPV6_FLAGS_O);
            }

            if (ret == 0)
            {
                eth_begin_send_frame(frame);
                wait(WAIT_ETH_SEND);
            }
        }
    }
    else
    {
        if (has_option && (icmp->flags & (ICMPV6_FLAGS_S | ICMPV6_FLAGS_O)))
        {
            // FIXME: do not always create the ARC entry.
            // Only do that after some solicitation is sent.
            ip_update_mac(target_addr, option_mac);
        }
        else
        {
            // Update ARC entry of target_addr without changing the MAC address.
            ip_update_mac_atime(target_addr);
        }
    }
    return 0;
}

int do_icmpv6_echo(frame_t *frame)
{
    // special path to send icmpv6 echo reply

    ipv6_header_t *ipv6 = (ipv6_header_t *)frame->payload;
    icmpv6_header_t *icmp = (icmpv6_header_t *)(ipv6 + 1);

    if (icmp->code != 0) return -EBADPKT;

    frame->meta.len = sizeof(eth_header_t) + sizeof(ipv6_header_t) + ntohs(ipv6->payload_len);

    memcpy(frame->dst_mac, frame->src_mac, 6);
    memcpy(frame->src_mac, &ifinfo.mac, 6);

    ipv6->hop_limit = DEFAULT_HOP_LIMIT;

    icmp->type = ICMPV6_TYPE_ECHO_REPLY;

    uint32_t checksum = ~icmp->checksum & 0xffff;
    ipv6_addr_t old_dst = ipv6->dst;
    ipv6->dst = ipv6->src;
    if (ipv6_is_multicast(old_dst))
    {
        ipv6->src = ifinfo.ipv6;
        checksum += ip_checksum_neg_partial(old_dst.u8, sizeof(old_dst.u8));
        checksum += ip_checksum_partial(ipv6->src.u8, sizeof(ipv6->src.u8));
    }
    else
    {
        ipv6->src = old_dst;
    }

    checksum += (~ICMPV6_TYPE_ECHO_REQUEST & 0xffff) + ICMPV6_TYPE_ECHO_REPLY;
    icmp->checksum = ip_checksum_final(checksum);

    eth_begin_send_frame(frame);
    wait(WAIT_ETH_SEND);
    return 0;
}

int handle_icmpv6(frame_t *frame)
{
    if (frame->meta.len < sizeof(eth_header_t) + sizeof(ipv6_header_t)
                          + sizeof(icmpv6_header_t)) return -EOOB;
    ipv6_header_t *ipv6 = (ipv6_header_t *)frame->payload;
    icmpv6_header_t *icmp = (icmpv6_header_t *)(ipv6 + 1);

    if (ipv6_l4_checksum(ipv6) != 0) return -EBADPKT;

    switch (icmp->type)
    {
    case ICMPV6_TYPE_NS:
    case ICMPV6_TYPE_NA:
        return handle_ndp(frame);
        break;
    case ICMPV6_TYPE_ECHO_REQUEST:
        return do_icmpv6_echo(frame);
        break;
    default:
        return -ENOIMPL;
        break;
    }
}

int handle_ipv6(frame_t *frame)
{
    if (frame->meta.len < sizeof(eth_header_t) + sizeof(ipv6_header_t)) return -EOOB;
    ipv6_header_t *ipv6 = (ipv6_header_t *)frame->payload;
    if (ip_version(ipv6) != 6) return -EBADPKT;
    if (frame->meta.len < sizeof(eth_header_t) + sizeof(ipv6_header_t)
                          + ntohs(ipv6->payload_len)) return -EOOB;
    frame->meta.len = sizeof(eth_header_t) + sizeof(ipv6_header_t)
                      + ntohs(ipv6->payload_len);

    bool is_to_me = ipv6_is_to_me(ipv6->dst);
    if (!is_to_me) return 0;

    switch (ipv6->next_header)
    {
    case IPV6_NEXT_HEADER_ICMP:
        return handle_icmpv6(frame);
        break;
    case IP_NEXT_HEADER_UDP:
    {
        uint64_t checksum = ipv6_l4_checksum(ipv6);
        if (checksum != 0) return -EBADPKT;
        return handle_udp(ipv6->src, (udp_header_t *)(ipv6 + 1),
                          frame->meta.len - (sizeof(eth_header_t) + sizeof(ipv6_header_t)));
        break;
    }
    default:
        return -ENOIMPL;
        break;
    }
}

__attribute__ ((aligned (16))) uint8_t udp_buff[4096];

int send_udp(uint16_t sport, ipv6_addr_t daddr, uint16_t dport, void *buff, size_t len)
{
    frame_t *const frame = (frame_t *)(udp_buff + ALIGN_OFFSET);
    const uint64_t frame_cap = sizeof(udp_buff) - ALIGN_OFFSET;

    if (frame_cap < sizeof(frame_t) + sizeof(ipv4_header_t)
                    + sizeof(udp_header_t) + len)
    {
        return -EOOB;
    }

    frame->meta.len = sizeof(eth_header_t) + sizeof(ipv4_header_t)
                      + sizeof(udp_header_t) + len;

    frame->ethertype = ETH_TYPE_IPV4;

    ipv4_header_t *ipv4 = (ipv4_header_t *)frame->payload;
    ipv4->version_header_len = 0x45;
    ipv4->dscp_ecn = 0;
    ipv4->total_len = htons(sizeof(ipv4_header_t) + sizeof(udp_header_t) + len);
    ipv4->id = 0;
    ipv4->flags = 0;
    ipv4->hop_limit = DEFAULT_HOP_LIMIT;
    ipv4->next_header = IP_NEXT_HEADER_UDP;
    ipv4->src = ifinfo.ipv4;
    ipv4->dst = extract_ipv4(daddr);

    ipv4->checksum = 0;
    ipv4->checksum = ip_checksum(ipv4, sizeof(ipv4_header_t));

    udp_header_t *udp = (udp_header_t *)(ipv4 + 1);

    udp->src_port = htons(sport);
    udp->dst_port = htons(dport);
    udp->len = htons(sizeof(udp_header_t) + len);

    void *payload = (void *)(udp + 1);
    memcpy(payload, buff, len);
    
    udp->checksum = 0;
    udp->checksum = ipv4_l4_checksum(ipv4, sizeof(udp_header_t) + len);

    return ip_send(frame, daddr);  // TODO: query route table
}

size_t recv_udp(uint16_t dport, ipv6_addr_t *saddr, uint16_t *sport, void *buff, size_t len,
                ipv6_addr_t expected_saddr, uint16_t expected_sport, uint64_t timeout)
{
    current->wait_packet.is_waiting = true;
    current->wait_packet.dport = dport;
    current->wait_packet.expected_saddr = expected_saddr;
    current->wait_packet.expected_sport = expected_sport;
    current->wait_packet.saddr = saddr;
    current->wait_packet.sport = sport;
    current->wait_packet.buff = buff;
    current->wait_packet.len = len;
    size_t out_len = 0;
    current->wait_packet.out_len = &out_len;
    current->wait_packet.start = rdcycle();
    current->wait_packet.delta = timeout;
    sched_yield();
    return out_len;
}

int handle_udp(ipv6_addr_t saddr, udp_header_t *udp, size_t len)
{
    if (len < sizeof(udp_header_t)) return -EOOB;
    size_t payload_len = ntohs(udp->len);
    if (len < payload_len) return -EOOB;
    if (payload_len < sizeof(udp_header_t)) return -EBADPKT;
    payload_len -= sizeof(udp_header_t);

    task_dispatch_udp(ntohs(udp->dst_port), saddr, ntohs(udp->src_port),
                      (void *)(udp + 1), payload_len);
    return 0;
}
