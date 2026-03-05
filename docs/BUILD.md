# Building MatryoshkaOS

This guide explains how to build and run MatryoshkaOS.

## Prerequisites

### Windows Host Setup

1. **WSL2 with Ubuntu** installed and running
2. **QEMU for Windows** (optional, for running from Windows)
3. **VMware Workstation Pro** (optional, for VMware testing)

### WSL2 Toolchain Setup

Open WSL2 terminal and run:

```bash
# Update package list
sudo apt update

# Install cross-compiler and assembler
sudo apt install -y gcc-x86-64-linux-gnu nasm binutils

# Install GRUB tools
sudo apt install -y grub-pc-bin grub-common xorriso

# Install QEMU for testing
sudo apt install -y qemu-system-x86

# Install testing tools
sudo apt install -y python3 python3-pytest python3-pexpect

# Install coverage tools
sudo apt install -y gcovr lcov

# Install debootstrap for rootfs
sudo apt install -y debootstrap
```

## Building the Kernel

### From WSL2

```bash
# Navigate to project directory
cd /mnt/c/myDesc/devSpace/matryoshka-os

# Build kernel
make all

# Expected output:
# AS   kernel/arch/x86_64/entry.asm
# CC   kernel/drivers/vga.c
# CC   kernel/kernel.c
# LD   build/matryoshka-kernel.elf
# ✓ Build complete: build/matryoshka-kernel.elf
```

### Build Targets

```bash
make all              # Build kernel (default)
make clean            # Clean build artifacts
make iso              # Create bootable ISO
make run              # Run in QEMU
make run-iso          # Run ISO in QEMU
make debug            # Run with GDB
make info             # Show kernel information
make help             # Show all targets
```

## Running the Kernel

### Option 1: Direct Kernel Boot in QEMU (Fastest)

```bash
make run
```

This will:
- Boot the kernel directly without ISO
- Display output in terminal
- Connect serial output to stdio

Expected output:
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

MatryoshkaOS - Built for System QA Portfolio
```

### Option 2: ISO Boot in QEMU

```bash
make iso
make run-iso
```

This creates a full bootable ISO with GRUB2 menu.

### Option 3: VMware Workstation

```bash
# Create ISO
make iso

# Copy ISO to Windows location
cp iso/matryoshka-os.iso /mnt/c/Users/YourUsername/Desktop/

# Then create new VM in VMware:
# - Type: Other Linux 5.x kernel 64-bit
# - Use ISO: matryoshka-os.iso
# - Memory: 512MB
# - Disk: 1GB (not used yet)
```

## Debugging

### With GDB in WSL2

Terminal 1 (QEMU):
```bash
make debug
# QEMU starts with GDB stub on port 1234
```

Terminal 2 (GDB):
```bash
gdb build/matryoshka-kernel.elf
(gdb) target remote :1234
(gdb) break kernel_main
(gdb) continue
(gdb) backtrace
```

### Useful GDB Commands

```
(gdb) break kernel_main    # Set breakpoint
(gdb) continue             # Continue execution
(gdb) next                 # Step over
(gdb) step                 # Step into
(gdb) backtrace            # Show call stack
(gdb) info registers       # Show CPU registers
(gdb) x/16xb address       # Examine memory
```

## Kernel Information

Check kernel details:

```bash
make info
```

Output shows:
- File type (ELF 64-bit)
- Sections (.text, .data, .bss)
- Size information

## Troubleshooting

### "Command not found: make"

Install build-essential:
```bash
sudo apt install build-essential
```

### "nasm: command not found"

Install NASM:
```bash
sudo apt install nasm
```

### "grub-mkrescue: command not found"

Install GRUB tools:
```bash
sudo apt install grub-pc-bin grub-common xorriso
```

### "No multiboot header found"

This means:
1. Entry point is wrong
2. Multiboot header is malformed
3. Linker script is incorrect

Check:
```bash
objdump -h build/matryoshka-kernel.elf
readelf -l build/matryoshka-kernel.elf
```

### QEMU shows blank screen

1. Check if kernel built successfully
2. Verify VGA output in serial console
3. Try adding `-serial mon:stdio` to QEMU command

## Next Steps

After successful "Hello World" boot:

1. **Memory Management** - Implement PMM, VMM, heap allocator
2. **Interrupts** - Setup GDT, IDT, PIC
3. **Processes** - Add scheduler and context switching
4. **Filesystem** - Implement VFS and ext2
5. **System Calls** - Linux-compatible syscall interface
6. **Testing** - Build comprehensive test suite

## Additional Resources

- [OSDev Wiki](https://wiki.osdev.org)
- [Intel SDM](https://software.intel.com/content/www/us/en/develop/articles/intel-sdm.html)
- [Multiboot2 Specification](https://www.gnu.org/software/grub/manual/multiboot2/)
- [x86_64 ABI](https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf)

