/**
 * MatryoshkaOS - Intel e1000 (82540EM) Ethernet Driver
 * Minimal driver for QEMU's default NIC.
 */

#ifndef MATRYOSHKA_E1000_H
#define MATRYOSHKA_E1000_H

#include <matryoshka/types.h>

#define E1000_VENDOR  0x8086
#define E1000_DEVICE  0x100E    /* 82540EM — QEMU default */

#define E1000_NUM_RX_DESC  32
#define E1000_NUM_TX_DESC  8
#define E1000_RX_BUF_SIZE  2048

int      e1000_init(void);
void     e1000_get_mac(uint8_t mac[6]);
int      e1000_send(const void *data, uint16_t len);
int      e1000_receive(void *buf, uint16_t buf_size);
bool     e1000_link_up(void);
uint32_t e1000_packets_rx(void);
uint32_t e1000_packets_tx(void);

#endif /* MATRYOSHKA_E1000_H */
