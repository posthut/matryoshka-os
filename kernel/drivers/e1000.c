/**
 * MatryoshkaOS - Intel e1000 (82540EM) Ethernet Driver
 *
 * Minimal send/receive over MMIO.  Works with QEMU's default NIC
 * (`-device e1000` or the implicit e1000 on the i440FX chipset).
 *
 * References:
 *   Intel 8254x Developer's Manual (PCI/PCI-X)
 *   OSDev Wiki: Intel Ethernet i217
 */

#include <matryoshka/e1000.h>
#include <matryoshka/pci.h>
#include <matryoshka/io.h>
#include <matryoshka/vmm.h>
#include <matryoshka/pmm.h>
#include <matryoshka/serial.h>
#include <matryoshka/string.h>
#include <matryoshka/vga.h>

/* ── E1000 register offsets ───────────────────────────────────────── */

#define REG_CTRL    0x0000
#define REG_STATUS  0x0008
#define REG_EECD    0x0010
#define REG_EEPROM  0x0014
#define REG_ICR     0x00C0
#define REG_IMS     0x00D0
#define REG_IMC     0x00D8
#define REG_RCTL    0x0100
#define REG_TCTL    0x0400

#define REG_RDBAL   0x2800
#define REG_RDBAH   0x2804
#define REG_RDLEN   0x2808
#define REG_RDH     0x2810
#define REG_RDT     0x2818

#define REG_TDBAL   0x3800
#define REG_TDBAH   0x3804
#define REG_TDLEN   0x3808
#define REG_TDH     0x3810
#define REG_TDT     0x3818

#define REG_RAL0    0x5400
#define REG_RAH0    0x5404
#define REG_MTA     0x5200

/* Control register bits */
#define CTRL_SLU    (1u << 6)    /* Set Link Up */
#define CTRL_RST    (1u << 26)   /* Device Reset */

/* Receive control bits */
#define RCTL_EN     (1u << 1)
#define RCTL_SBP    (1u << 2)
#define RCTL_UPE    (1u << 3)
#define RCTL_MPE    (1u << 4)
#define RCTL_BAM    (1u << 15)
#define RCTL_BSIZE_2048 (0u << 16)
#define RCTL_SECRC  (1u << 26)

/* Transmit control bits */
#define TCTL_EN     (1u << 1)
#define TCTL_PSP    (1u << 3)
#define TCTL_CT_SHIFT  4
#define TCTL_COLD_SHIFT 12

/* TX descriptor command bits */
#define TDESC_CMD_EOP  (1u << 0)
#define TDESC_CMD_IFCS (1u << 1)
#define TDESC_CMD_RS   (1u << 3)
#define TDESC_STA_DD   (1u << 0)

/* RX descriptor status bits */
#define RDESC_STA_DD   (1u << 0)
#define RDESC_STA_EOP  (1u << 1)

/* ── Descriptor structures ────────────────────────────────────────── */

typedef struct {
    uint32_t addr_lo;
    uint32_t addr_hi;
    uint16_t length;
    uint16_t checksum;
    uint8_t  status;
    uint8_t  errors;
    uint16_t special;
} __attribute__((packed)) e1000_rx_desc_t;

typedef struct {
    uint32_t addr_lo;
    uint32_t addr_hi;
    uint16_t length;
    uint8_t  cso;
    uint8_t  cmd;
    uint8_t  status;
    uint8_t  css;
    uint16_t special;
} __attribute__((packed)) e1000_tx_desc_t;

/* ── Driver state ─────────────────────────────────────────────────── */

static volatile uint32_t *mmio_base;
static uint8_t  mac_addr[6];
static bool     initialised;

static e1000_rx_desc_t rx_descs[E1000_NUM_RX_DESC]
    __attribute__((aligned(16)));
static e1000_tx_desc_t tx_descs[E1000_NUM_TX_DESC]
    __attribute__((aligned(16)));

static uint8_t rx_buffers[E1000_NUM_RX_DESC][E1000_RX_BUF_SIZE]
    __attribute__((aligned(16)));

static uint32_t rx_cur;
static uint32_t tx_cur;
static uint32_t stat_rx;
static uint32_t stat_tx;

/* ── MMIO helpers ─────────────────────────────────────────────────── */

static uint32_t e1000_read(uint32_t reg) {
    return mmio_base[reg / 4];
}

static void e1000_write(uint32_t reg, uint32_t val) {
    mmio_base[reg / 4] = val;
}

/* ── MAC address ──────────────────────────────────────────────────── */

static bool read_mac_eeprom(void) {
    for (int i = 0; i < 3; i++) {
        e1000_write(REG_EEPROM, 1 | ((uint32_t)i << 8));
        uint32_t val;
        int timeout = 10000;
        do {
            val = e1000_read(REG_EEPROM);
        } while (!(val & (1 << 4)) && --timeout > 0);
        if (timeout <= 0) return false;
        mac_addr[i * 2]     = (uint8_t)(val >> 16);
        mac_addr[i * 2 + 1] = (uint8_t)(val >> 24);
    }
    return true;
}

static void read_mac_ral(void) {
    uint32_t lo = e1000_read(REG_RAL0);
    uint32_t hi = e1000_read(REG_RAH0);
    mac_addr[0] = (uint8_t)(lo);
    mac_addr[1] = (uint8_t)(lo >> 8);
    mac_addr[2] = (uint8_t)(lo >> 16);
    mac_addr[3] = (uint8_t)(lo >> 24);
    mac_addr[4] = (uint8_t)(hi);
    mac_addr[5] = (uint8_t)(hi >> 8);
}

/* ── Ring setup ───────────────────────────────────────────────────── */

static void init_rx(void) {
    for (int i = 0; i < E1000_NUM_RX_DESC; i++) {
        rx_descs[i].addr_lo = (uint32_t)(uintptr_t)rx_buffers[i];
        rx_descs[i].addr_hi = 0;
        rx_descs[i].status  = 0;
    }

    e1000_write(REG_RDBAL, (uint32_t)(uintptr_t)rx_descs);
    e1000_write(REG_RDBAH, 0);
    e1000_write(REG_RDLEN, E1000_NUM_RX_DESC * sizeof(e1000_rx_desc_t));
    e1000_write(REG_RDH, 0);
    e1000_write(REG_RDT, E1000_NUM_RX_DESC - 1);
    rx_cur = 0;

    e1000_write(REG_RCTL,
                RCTL_EN | RCTL_BAM | RCTL_BSIZE_2048 | RCTL_SECRC);
}

static void init_tx(void) {
    memset(tx_descs, 0, sizeof(tx_descs));

    e1000_write(REG_TDBAL, (uint32_t)(uintptr_t)tx_descs);
    e1000_write(REG_TDBAH, 0);
    e1000_write(REG_TDLEN, E1000_NUM_TX_DESC * sizeof(e1000_tx_desc_t));
    e1000_write(REG_TDH, 0);
    e1000_write(REG_TDT, 0);
    tx_cur = 0;

    e1000_write(REG_TCTL,
                TCTL_EN | TCTL_PSP
                | (15u << TCTL_CT_SHIFT)
                | (64u << TCTL_COLD_SHIFT));
}

/* ── Public API ───────────────────────────────────────────────────── */

int e1000_init(void) {
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Initializing e1000 network driver...\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    pci_device_t dev;
    if (!pci_find_device(E1000_VENDOR, E1000_DEVICE, &dev)) {
        vga_puts("  e1000 NIC not found on PCI bus\n\n");
        klog("e1000: NIC not found");
        return -1;
    }

    klog("e1000: found on PCI bus");

    /* Enable PCI bus mastering + memory space */
    uint16_t cmd = pci_read16(dev.bus, dev.slot, dev.func, PCI_COMMAND);
    cmd |= PCI_CMD_MEMORY | PCI_CMD_MASTER;
    pci_write16(dev.bus, dev.slot, dev.func, PCI_COMMAND, cmd);

    mmio_base = (volatile uint32_t *)(uintptr_t)dev.bar0;

    /* Identity-map the MMIO region (128KB should be enough) */
    for (uint32_t off = 0; off < 0x20000; off += PAGE_SIZE) {
        vmm_map(dev.bar0 + off, dev.bar0 + off,
                PTE_PRESENT | PTE_WRITABLE | PTE_CACHE_DISABLE);
    }

    /* Software reset */
    e1000_write(REG_CTRL, e1000_read(REG_CTRL) | CTRL_RST);
    for (volatile int i = 0; i < 100000; i++);
    e1000_write(REG_IMC, 0xFFFFFFFF);   /* mask all interrupts for now */
    e1000_read(REG_ICR);                 /* clear pending */

    /* Read MAC address */
    if (!read_mac_eeprom())
        read_mac_ral();

    /* Clear multicast table */
    for (int i = 0; i < 128; i++)
        e1000_write(REG_MTA + i * 4, 0);

    init_rx();
    init_tx();

    /* Set link up */
    e1000_write(REG_CTRL,
                e1000_read(REG_CTRL) | CTRL_SLU);

    initialised = true;
    stat_rx = 0;
    stat_tx = 0;

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] e1000 at BAR0=0x");
    /* quick hex print */
    {
        const char hex[] = "0123456789ABCDEF";
        char buf[9];
        uint32_t v = dev.bar0;
        for (int i = 7; i >= 0; i--) { buf[i] = hex[v & 0xF]; v >>= 4; }
        buf[8] = '\0';
        vga_puts(buf);
    }
    vga_puts(", IRQ ");
    {
        char b[4]; int n = dev.irq_line;
        b[0] = '0' + n / 10; b[1] = '0' + n % 10; b[2] = '\0';
        vga_puts(n >= 10 ? b : b + 1);
    }
    vga_putchar('\n');

    vga_puts("  [OK] MAC ");
    for (int i = 0; i < 6; i++) {
        const char hex[] = "0123456789ABCDEF";
        vga_putchar(hex[mac_addr[i] >> 4]);
        vga_putchar(hex[mac_addr[i] & 0xF]);
        if (i < 5) vga_putchar(':');
    }
    vga_putchar('\n');

    uint32_t status = e1000_read(REG_STATUS);
    if (status & 0x02) {
        vga_puts("  [OK] Link UP\n\n");
        klog("e1000: link up");
    } else {
        vga_puts("  [--] Link DOWN\n\n");
        klog("e1000: link down");
    }

    return 0;
}

void e1000_get_mac(uint8_t out[6]) {
    memcpy(out, mac_addr, 6);
}

bool e1000_link_up(void) {
    if (!initialised) return false;
    return (e1000_read(REG_STATUS) & 0x02) != 0;
}

int e1000_send(const void *data, uint16_t len) {
    if (!initialised || len > 1518) return -1;

    e1000_tx_desc_t *desc = &tx_descs[tx_cur];
    desc->addr_lo = (uint32_t)(uintptr_t)data;
    desc->addr_hi = 0;
    desc->length  = len;
    desc->cmd     = TDESC_CMD_EOP | TDESC_CMD_IFCS | TDESC_CMD_RS;
    desc->status  = 0;

    uint32_t old = tx_cur;
    tx_cur = (tx_cur + 1) % E1000_NUM_TX_DESC;
    e1000_write(REG_TDT, tx_cur);

    /* Busy-wait for completion (simple approach) */
    int timeout = 100000;
    while (!(tx_descs[old].status & TDESC_STA_DD) && --timeout > 0);
    if (timeout <= 0) return -1;

    stat_tx++;
    return (int)len;
}

int e1000_receive(void *buf, uint16_t buf_size) {
    if (!initialised) return -1;

    e1000_rx_desc_t *desc = &rx_descs[rx_cur];
    if (!(desc->status & RDESC_STA_DD)) return 0;

    uint16_t len = desc->length;
    if (len > buf_size) len = buf_size;
    memcpy(buf, rx_buffers[rx_cur], len);

    desc->status = 0;
    uint32_t old = rx_cur;
    rx_cur = (rx_cur + 1) % E1000_NUM_RX_DESC;
    e1000_write(REG_RDT, old);

    stat_rx++;
    return (int)len;
}

uint32_t e1000_packets_rx(void) { return stat_rx; }
uint32_t e1000_packets_tx(void) { return stat_tx; }
