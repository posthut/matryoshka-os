/**
 * MatryoshkaOS - Minimal Network Stack
 *
 * Handles incoming Ethernet frames:
 *   - ARP requests  → sends ARP replies
 *   - ICMP echo     → sends ICMP echo reply (ping)
 *
 * Just enough to respond to `ping` from the host.
 */

#include <matryoshka/net.h>
#include <matryoshka/e1000.h>
#include <matryoshka/serial.h>
#include <matryoshka/string.h>
#include <matryoshka/vga.h>

static uint8_t our_mac[6];
static uint8_t our_ip[4] = {10, 0, 2, 15};   /* QEMU default user-net */

/* ── Helpers ──────────────────────────────────────────────────────── */

static uint16_t ip_checksum(const void *data, size_t len) {
    const uint16_t *p = (const uint16_t *)data;
    uint32_t sum = 0;
    while (len > 1) { sum += *p++; len -= 2; }
    if (len) sum += *(const uint8_t *)p;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

/* ── ARP ──────────────────────────────────────────────────────────── */

static void handle_arp(const uint8_t *frame, uint16_t len) {
    if (len < sizeof(eth_header_t) + sizeof(arp_packet_t)) return;
    const arp_packet_t *arp = (const arp_packet_t *)(frame + sizeof(eth_header_t));

    if (ntohs(arp->operation) != ARP_OP_REQUEST) return;
    if (memcmp(arp->target_ip, our_ip, 4) != 0) return;

    klog("net: ARP request for us, sending reply");

    uint8_t reply[sizeof(eth_header_t) + sizeof(arp_packet_t)];
    eth_header_t *eth = (eth_header_t *)reply;
    arp_packet_t *rep = (arp_packet_t *)(reply + sizeof(eth_header_t));

    memcpy(eth->dst, arp->sender_mac, 6);
    memcpy(eth->src, our_mac, 6);
    eth->ethertype = htons(ETH_TYPE_ARP);

    rep->hw_type    = htons(1);
    rep->proto_type = htons(0x0800);
    rep->hw_len     = 6;
    rep->proto_len  = 4;
    rep->operation  = htons(ARP_OP_REPLY);
    memcpy(rep->sender_mac, our_mac, 6);
    memcpy(rep->sender_ip,  our_ip, 4);
    memcpy(rep->target_mac, arp->sender_mac, 6);
    memcpy(rep->target_ip,  arp->sender_ip, 4);

    e1000_send(reply, sizeof(reply));
}

/* ── ICMP ─────────────────────────────────────────────────────────── */

static void handle_icmp(const uint8_t *frame, uint16_t len) {
    const eth_header_t *eth = (const eth_header_t *)frame;
    const ip_header_t  *ip  = (const ip_header_t *)(frame + sizeof(eth_header_t));

    uint16_t ip_hdr_len = (ip->ver_ihl & 0x0F) * 4;
    uint16_t ip_total   = ntohs(ip->total_length);
    if (len < sizeof(eth_header_t) + ip_hdr_len + sizeof(icmp_header_t))
        return;

    const icmp_header_t *icmp =
        (const icmp_header_t *)(frame + sizeof(eth_header_t) + ip_hdr_len);

    if (icmp->type != ICMP_ECHO_REQUEST) return;

    klog("net: ICMP echo request, sending reply");

    uint16_t reply_len = sizeof(eth_header_t) + ip_total;
    if (reply_len > 1518) return;

    /* Build reply in-place on a static buffer */
    static uint8_t reply[1518];
    memcpy(reply, frame, reply_len);

    eth_header_t *reth = (eth_header_t *)reply;
    ip_header_t  *rip  = (ip_header_t *)(reply + sizeof(eth_header_t));
    icmp_header_t *ricmp =
        (icmp_header_t *)(reply + sizeof(eth_header_t) + ip_hdr_len);

    /* Swap MACs */
    memcpy(reth->dst, eth->src, 6);
    memcpy(reth->src, our_mac, 6);

    /* Swap IPs */
    memcpy(rip->dst_ip, ip->src_ip, 4);
    memcpy(rip->src_ip, our_ip, 4);
    rip->ttl = 64;
    rip->checksum = 0;
    rip->checksum = ip_checksum(rip, ip_hdr_len);

    /* ICMP echo reply */
    ricmp->type = ICMP_ECHO_REPLY;
    ricmp->checksum = 0;
    uint16_t icmp_len = ip_total - ip_hdr_len;
    ricmp->checksum = ip_checksum(ricmp, icmp_len);

    e1000_send(reply, reply_len);
}

/* ── IPv4 dispatch ────────────────────────────────────────────────── */

static void handle_ip(const uint8_t *frame, uint16_t len) {
    if (len < sizeof(eth_header_t) + sizeof(ip_header_t)) return;
    const ip_header_t *ip = (const ip_header_t *)(frame + sizeof(eth_header_t));

    if ((ip->ver_ihl >> 4) != 4) return;
    if (memcmp(ip->dst_ip, our_ip, 4) != 0) return;

    switch (ip->protocol) {
    case IP_PROTO_ICMP:
        handle_icmp(frame, len);
        break;
    default:
        break;
    }
}

/* ── Public API ───────────────────────────────────────────────────── */

void net_init(void) {
    e1000_get_mac(our_mac);
    klog("net: stack initialized");
}

void net_set_ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    our_ip[0] = a; our_ip[1] = b; our_ip[2] = c; our_ip[3] = d;
}

void net_get_ip(uint8_t ip[4]) {
    memcpy(ip, our_ip, 4);
}

void net_poll(void) {
    static uint8_t buf[2048];
    int len = e1000_receive(buf, sizeof(buf));
    if (len <= 0) return;
    if ((uint16_t)len < sizeof(eth_header_t)) return;

    const eth_header_t *eth = (const eth_header_t *)buf;
    uint16_t type = ntohs(eth->ethertype);

    switch (type) {
    case ETH_TYPE_ARP:
        handle_arp(buf, (uint16_t)len);
        break;
    case ETH_TYPE_IP:
        handle_ip(buf, (uint16_t)len);
        break;
    }
}
