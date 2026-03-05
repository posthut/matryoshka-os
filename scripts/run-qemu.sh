#!/bin/bash
# MatryoshkaOS - Run in QEMU (no timeout)
# Press Ctrl+A then X to exit QEMU

cd /mnt/c/myDesc/devSpace/matryoshka-os

echo "🚀 Starting MatryoshkaOS in QEMU..."
echo "   QEMU window will open"
echo "   Select 'MatryoshkaOS' in GRUB menu"
echo "   Press Ctrl+A then X to exit QEMU"
echo ""

qemu-system-x86_64 \
    -cdrom iso/matryoshka-os.iso \
    -m 512M \
    -serial stdio \
    -no-reboot \
    -no-shutdown

