/**
 * MatryoshkaOS - Minimal Network Stack
 * Ethernet, ARP, IPv4, ICMP (ping reply)
 */

#ifndef MATRYOSHKA_NET_H
#define MATRYOSHKA_NET_H

#include <matryoshka/types.h>

/* Ethernet */
#define ETH_TYPE_ARP  0x0806
#define ETH_TYPE_IP   0x0800

typedef struct {
    uint8_t  dst[6];
    uint8_t  src[6];
    uint16_t ethertype;     /* big-endian */
} __attribute__((packed)) eth_header_t;

/* ARP */
#define ARP_OP_REQUEST  1
#define ARP_OP_REPLY    2

typedef struct {
    uint16_t hw_type;       /* 1 = Ethernet */
    uint16_t proto_type;    /* 0x0800 = IPv4 */
    uint8_t  hw_len;        /* 6 */
    uint8_t  proto_len;     /* 4 */
    uint16_t operation;
    uint8_t  sender_mac[6];
    uint8_t  sender_ip[4];
    uint8_t  target_mac[6];
    uint8_t  target_ip[4];
} __attribute__((packed)) arp_packet_t;

/* IPv4 */
#define IP_PROTO_ICMP  1
#define IP_PROTO_UDP   17

typedef struct {
    uint8_t  ver_ihl;
    uint8_t  tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint8_t  src_ip[4];
    uint8_t  dst_ip[4];
} __attribute__((packed)) ip_header_t;

/* ICMP */
#define ICMP_ECHO_REPLY   0
#define ICMP_ECHO_REQUEST 8

typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
} __attribute__((packed)) icmp_header_t;

/* Byte-order helpers */
static inline uint16_t htons(uint16_t v) { return (v >> 8) | (v << 8); }
static inline uint16_t ntohs(uint16_t v) { return htons(v); }

void net_init(void);
void net_poll(void);
void net_set_ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
void net_get_ip(uint8_t ip[4]);

#endif /* MATRYOSHKA_NET_H */
