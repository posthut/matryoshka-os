/**
 * MatryoshkaOS - PCI Configuration Space Access
 */

#include <matryoshka/pci.h>
#include <matryoshka/io.h>

static uint32_t pci_addr(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off) {
    return 0x80000000u
         | ((uint32_t)bus  << 16)
         | ((uint32_t)slot << 11)
         | ((uint32_t)func <<  8)
         | (off & 0xFC);
}

uint32_t pci_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off) {
    outl(PCI_CONFIG_ADDR, pci_addr(bus, slot, func, off));
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off) {
    uint32_t val = pci_read32(bus, slot, func, off & ~3);
    return (uint16_t)(val >> ((off & 2) * 8));
}

void pci_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off, uint32_t val) {
    outl(PCI_CONFIG_ADDR, pci_addr(bus, slot, func, off));
    outl(PCI_CONFIG_DATA, val);
}

void pci_write16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t off, uint16_t val) {
    uint32_t old = pci_read32(bus, slot, func, off & ~3);
    int shift = (off & 2) * 8;
    old &= ~(0xFFFF << shift);
    old |= (uint32_t)val << shift;
    pci_write32(bus, slot, func, off & ~3, old);
}

bool pci_find_device(uint16_t vendor, uint16_t device, pci_device_t *dev) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint16_t vid = pci_read16(bus, slot, 0, PCI_VENDOR_ID);
            if (vid == 0xFFFF) continue;
            uint16_t did = pci_read16(bus, slot, 0, PCI_DEVICE_ID);
            if (vid == vendor && did == device) {
                dev->bus       = (uint8_t)bus;
                dev->slot      = slot;
                dev->func      = 0;
                dev->vendor_id = vid;
                dev->device_id = did;
                dev->class_code = (uint8_t)(pci_read32(bus, slot, 0, 0x08) >> 24);
                dev->subclass   = (uint8_t)(pci_read32(bus, slot, 0, 0x08) >> 16);
                dev->bar0      = pci_read32(bus, slot, 0, PCI_BAR0) & ~0xFu;
                dev->irq_line  = (uint8_t)pci_read32(bus, slot, 0, PCI_IRQ_LINE);
                return true;
            }
        }
    }
    return false;
}
