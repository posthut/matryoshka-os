/**
 * MatryoshkaOS - PCI Bus Configuration
 * Minimal PCI access for device discovery.
 */

#ifndef MATRYOSHKA_PCI_H
#define MATRYOSHKA_PCI_H

#include <matryoshka/types.h>

#define PCI_CONFIG_ADDR  0xCF8
#define PCI_CONFIG_DATA  0xCFC

#define PCI_VENDOR_ID    0x00
#define PCI_DEVICE_ID    0x02
#define PCI_COMMAND      0x04
#define PCI_STATUS       0x06
#define PCI_CLASS        0x0B
#define PCI_SUBCLASS     0x0A
#define PCI_BAR0         0x10
#define PCI_BAR1         0x14
#define PCI_IRQ_LINE     0x3C

#define PCI_CMD_IO       0x0001
#define PCI_CMD_MEMORY   0x0002
#define PCI_CMD_MASTER   0x0004

typedef struct {
    uint8_t  bus, slot, func;
    uint16_t vendor_id, device_id;
    uint8_t  class_code, subclass;
    uint8_t  irq_line;
    uint32_t bar0;
} pci_device_t;

uint32_t pci_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void     pci_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val);
void     pci_write16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t val);

/**
 * Scan PCI bus for a device with given vendor/device IDs.
 * Returns true if found and fills @a dev.
 */
bool pci_find_device(uint16_t vendor, uint16_t device, pci_device_t *dev);

#endif /* MATRYOSHKA_PCI_H */
