# Quick Start Guide

Get MatryoshkaOS running in 5 minutes.

## Step 1: Install Prerequisites (WSL2)

```bash
sudo apt update && sudo apt install -y \
    gcc-x86-64-linux-gnu \
    nasm \
    binutils \
    grub-pc-bin \
    grub-common \
    xorriso \
    qemu-system-x86 \
    make
```

## Step 2: Navigate to Project

```bash
cd /mnt/c/myDesc/devSpace/matryoshka-os
```

## Step 3: Build Kernel

```bash
make all
```

Expected output:
```
AS   kernel/arch/x86_64/entry.asm
CC   kernel/drivers/vga.c
CC   kernel/kernel.c
LD   build/matryoshka-kernel.elf
✓ Build complete
```

## Step 4: Run in QEMU

```bash
make run
```

You should see:
```
MatryoshkaOS v1.0
====================================

Kernel booted successfully!

System Information:
  - Architecture: x86_64
  - Bootloader: GRUB2 Multiboot2
  - VGA Mode: 80x25 text mode

Status:
  [OK] VGA driver initialized
  [OK] Kernel running in long mode
```

**Press Ctrl+C to exit QEMU**

## Step 5: Create Bootable ISO (Optional)

```bash
make iso
make run-iso
```

The ISO file will be in `iso/matryoshka-os.iso` and can be used with VMware.

---

## Alternative Commands

Both command formats work identically:

```bash
# Using full name
matryoshka build
matryoshka run

# Using short alias (requires installation)
mshka build
mshka run
```

## What's Next?

See [Build Guide](BUILD.md) for:
- Debugging with GDB
- Running on VMware
- Testing
- Troubleshooting

## Common Issues

**"make: command not found"**
```bash
sudo apt install build-essential
```

**"nasm: command not found"**
```bash
sudo apt install nasm
```

**QEMU shows blank screen**
- Check build was successful
- Try: `make clean && make all && make run`

