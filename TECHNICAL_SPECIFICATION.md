# MatryoshkaOS — Technical Specification

**Version:** 1.1 | **Date:** March 2026  
**Goal:** Educational OS for System QA Engineer Portfolio  
**Context:** Architectural decisions made with enterprise/banking environment requirements in mind

---

## Project Naming Convention

```
┌─────────────────────────────────────────────────┐
│ NAMING STRUCTURE                                │
├─────────────────────────────────────────────────┤
│ GitHub repo:        matryoshka-os               │
│ Branding:           MatryoshkaOS                │
│                                                 │
│ CLI commands:       matryoshka <command>  (full)│
│            OR:      mshka <command>       (short)│
│                                                 │
│ Binaries:           matryoshka-kernel.elf       │
│                     mshka-kernel.elf      (alias)│
│                                                 │
│ Code prefix:        matryoshka_ / MATRYOSHKA_   │
│ Short prefix:       mshka_ / MSHKA_       (opt) │
│                                                 │
│ Package name:       matryoshka-os-tools         │
│ Docker image:       matryoshka-os:latest        │
│ ISO file:           matryoshka-os-v1.0.iso      │
└─────────────────────────────────────────────────┘
```

**Why two variants?**
- `matryoshka` — official, full name for documentation and branding
- `mshka` — short, convenient for daily work (pronounced like "mishka")
- Both commands work identically, user chooses what's more comfortable

---

## 1. Project Goal

MatryoshkaOS is a minimal viable operating system with two objectives:

1. Architecturally clean kernel in C + ASM from scratch
2. Professional testing infrastructure for learning QA at system level

The project is published on GitHub as a portfolio for **System QA Engineer (sQA)** position.

---

## 2. Architecture

```
┌─────────────────────────────────────────┐
│  Userspace: bash, vim, ssh, apt         │  ← Debian base (ready-made)
├─────────────────────────────────────────┤
│  Init (PID 1)                           │  ← We write (~100 lines C)
├─────────────────────────────────────────┤
│  Syscall Interface (Linux-compatible)   │  ← We write
├─────────────────────────────────────────┤
│  Kernel: mm, proc, fs, drivers, net     │  ← We write
├─────────────────────────────────────────┤
│  Bootloader: GRUB2                      │  ← Ready-made (rationale below)
└─────────────────────────────────────────┘
```

### Why GRUB2 Instead of Custom Bootloader

This is a deliberate architectural decision made for the following reasons:

- **Security:** 20+ years of public auditing, closed CVEs, known vulnerability history
- **Secure Boot:** UEFI Secure Boot support out of the box — mandatory requirement in banking infrastructure
- **Regulatory Compliance:** Central Bank of Russia, PCI DSS require justification for non-standard solutions — GRUB2 is an industry standard
- **Priorities:** Resources directed to kernel and testing infrastructure, not to an already solved problem

> This is exactly how decisions are made in production environment of large banks (Sberbank, VTB, Tinkoff): use audited components where justified, and custom development where it provides real value.

**Key Decision:** The kernel implements Linux-compatible syscall ABI — Debian binaries
(vim, openssh, apt) run on top of our kernel without modifications. This is exactly how
all enterprise Linux distributions work, including SberLinux (based on Debian/Ubuntu).

### Repository Structure

```
matryoshka-os/
├── kernel/
│   ├── arch/x86_64/     # ASM: entry point, GDT, IDT, context switch
│   ├── mm/              # Memory management (PMM, VMM, heap)
│   ├── proc/            # Processes and scheduler
│   ├── fs/              # Filesystem (VFS, ext2, initrd)
│   ├── drivers/         # Drivers (VGA, serial, keyboard, ATA, e1000)
│   ├── net/             # Network stack (Ethernet, ARP, IP, TCP, UDP)
│   └── syscall/         # System call table
├── userspace/
│   └── init.c           # PID 1
├── rootfs/
│   └── build.sh         # Build Debian rootfs
├── tests/
│   ├── unit/            # Unity C tests
│   ├── integration/     # Integration tests
│   ├── system/          # pytest + pexpect (end-to-end)
│   └── regression/      # Smoke tests for CI
├── tools/               # Helper scripts
├── docs/                # Documentation
├── Makefile
└── README.md
```

---

## 3. Kernel Components (written from scratch)

### 3.1 arch/x86_64/
- `entry.asm` — entry point, stack setup, jump to C
- `gdt.c` — Global Descriptor Table
- `idt.c` — Interrupt Descriptor Table
- `pic.c` — Programmable Interrupt Controller (8259A)
- `io.h` — inline inb/outb

### 3.2 mm/ — Memory Management
- `pmm.c` — Physical Memory Manager: frame bitmap
- `vmm.c` — Virtual Memory Manager: paging, 4KB pages
- `heap.c` — kmalloc / kfree

### 3.3 proc/ — Processes
- `process.c` — PCB: pid, state, stack, page table
- `scheduler.c` — Round-robin scheduler with priorities
- `context.asm` — Context switching
- `elf.c` — ELF binary loader

### 3.4 fs/ — Filesystem
- `vfs.c` — Virtual File System: open, read, write, close
- `ext2.c` — ext2 implementation
- `initrd.c` — RAM disk for initial boot

### 3.5 drivers/ — Drivers
- `serial.c` — UART/COM (debug output)
- `vga.c` — VGA text mode
- `keyboard.c` — PS/2 keyboard
- `ata.c` — ATA/IDE controller
- `pci.c` — PCI device enumeration
- `e1000.c` — Intel e1000 network card (supported by VMware + QEMU)

### 3.6 net/ — Network Stack
- `ethernet.c`, `arp.c`, `ip.c`, `tcp.c`, `udp.c`

### 3.7 syscall/ — System Calls

Minimal set for running Debian userspace (via SYSCALL/SYSRET x86_64):

| # | Syscall | Needed for |
|---|---------|-----------|
| 0 | read | bash, vim, ssh |
| 1 | write | bash, vim, ssh |
| 2 | open | all programs |
| 3 | close | all programs |
| 9 | mmap | glibc, ELF loading |
| 11 | munmap | glibc |
| 12 | brk | malloc |
| 57 | fork | bash |
| 59 | execve | bash, init |
| 60 | exit | all programs |
| 61 | wait4 | bash, init |
| 22 | pipe | bash (pipe \|) |
| 33 | dup2 | bash (>, <) |
| 41 | socket | ssh, apt |
| 42 | connect | ssh, apt |

---

## 4. Userspace

### 4.1 Init (we write)

`userspace/init.c` — PID 1, ~100 lines:
- Mounts /proc, /sys, /dev
- Launches /bin/bash
- wait() loop — reaps zombie processes
- On SIGCHLD restarts crashed shell

### 4.2 Debian rootfs (ready-made)

Script `rootfs/build.sh`:
```bash
debootstrap --variant=minbase stable ./rootfs http://deb.debian.org/debian
chroot rootfs apt install -y vim openssh-server coreutils iproute2 curl python3
# copy our init as /sbin/init
# pack into ext2 image
```

Packages: `bash`, `vim`, `openssh-server`, `apt`, `coreutils`,
`util-linux`, `iproute2`, `curl`, `python3`

> **Note:** Debian was chosen deliberately — SberLinux, used in Sberbank,
> is based on Debian/Ubuntu. Package base and apt compatibility is not a coincidence.

---

## 5. Testing Infrastructure

### 5.1 Unit Tests (tests/unit/)

Framework: **Unity (C)**. Compiled for host, without running the OS.

| Module | Test | Expected Result |
|--------|------|-----------------|
| pmm | test_alloc_single_frame | Non-zero frame address |
| pmm | test_alloc_all_frames | NULL on OOM |
| pmm | test_free_and_realloc | After free frame available again |
| heap | test_kmalloc_basic | Aligned pointer |
| heap | test_kmalloc_zero | Does not crash on kmalloc(0) |
| heap | test_kfree_double | Double free does not cause panic |
| vfs | test_open_nonexistent | Returns ENOENT |
| vfs | test_write_read | Data reads without changes |
| scheduler | test_round_robin_order | Processes get CPU in order |
| elf | test_valid_elf_header | Correct ELF parses without errors |
| elf | test_invalid_magic | Incorrect magic = error |

Run: `make test-unit`

### 5.2 Integration Tests (tests/integration/)

Run inside kernel in special test mode (kernel boot param `test=1`).

- `test_process_lifecycle` — fork + execve + wait completes correctly
- `test_pipe_communication` — process writes to pipe, another reads
- `test_file_persistence` — file exists after FS reinitialization
- `test_memory_isolation` — process A cannot read process B's memory
- `test_signal_delivery` — SIGKILL terminates, SIGCHLD delivered to parent

Run: `make test-integration`

### 5.3 System / End-to-End Tests (tests/system/)

Stack: **Python 3 + pytest + pexpect**. Launch QEMU, control via serial/SSH.

| Test | Action | PASS Criteria |
|------|--------|---------------|
| test_boot_success | Launch ISO in QEMU | Login prompt in < 30 sec |
| test_shell_available | Login as root | bash prompt appeared |
| test_vim_launches | Launch vim | vim opened without segfault |
| test_file_create | echo hello > /tmp/f | cat /tmp/f = "hello" |
| test_ssh_connect | ssh localhost | Successful connection |
| test_apt_update | apt update | Return code 0 |
| test_apt_install | apt install htop | htop runs |
| test_network_ping | ping 8.8.8.8 -c1 | Received 1 reply |
| test_process_list | ps aux | Contains init and bash |
| test_reboot | reboot | OS rebooted |

Run: `make test-system`

### 5.4 Regression Tests / CI (tests/regression/)

GitHub Actions on every PR and push to main:
- All unit tests
- Smoke tests (boot + shell + vim)
- Kernel size control (±10% from previous commit = warning)
- Check for absence of PANIC in serial log

### 5.5 Code Coverage

`gcov` + `lcov` for unit tests. Goal: **>= 70%** for key kernel modules.

---

## 6. Development Environment

### Host
- Windows 11 Pro x64
- VMware Workstation Pro — primary launch and testing
- QEMU for Windows — debugging via GDB stub (`qemu ... -s -S`)
- WSL2 (Ubuntu) — kernel compilation and rootfs build
- Cursor IDE + Claude — development
- Git + GitHub — version control and CI/CD

### Toolchain (in WSL2)
```bash
# Cross-compiler
sudo apt install gcc-x86-64-linux-gnu nasm binutils grub-pc-bin grub-common xorriso

# For rootfs
sudo apt install debootstrap

# For tests
sudo apt install python3 python3-pytest python3-pexpect qemu-system-x86

# Coverage
sudo apt install gcovr lcov
```

### Target Architecture
- x86_64 (64-bit)
- Bootloader: GRUB2 Multiboot2
- Kernel format: ELF64
- Disk image: ext2 in vmdk/qcow2

---

## 7. Development Stages

| # | Stage | Key Tasks | Result |
|---|-------|-----------|--------|
| 1 | Environment | WSL2, cross-compiler, QEMU, VMware | Toolchain ready |
| 2 | Minimal kernel | GRUB2, entry.asm, VGA "Hello World" | OS boots |
| 3 | Memory | PMM, VMM, paging, kmalloc | Dynamic memory works |
| 4 | Interrupts | GDT, IDT, PIC, timer, keyboard | IRQ processed |
| 5 | Processes | PCB, scheduler, context switch, ELF | fork/exec works |
| 6 | Filesystem | VFS, ext2, initrd | Files read from disk |
| 7 | Syscalls | Syscall table, Linux ABI | Debian binaries start |
| 8 | Network | e1000 driver, TCP/IP stack | ping works |
| 9 | Userspace | init.c, Debian rootfs, vim/ssh/apt | Full-featured shell |
| 10 | Tests | Unit, integration, system, CI/CD | Testing infra ready |

---

## 8. Definition of Done

### Kernel
- [ ] Boots via GRUB2 on VMware and QEMU
- [ ] Initializes memory, interrupts, processes, FS
- [ ] Implements Linux-compatible syscall ABI
- [ ] Launches /sbin/init from Debian rootfs
- [ ] Does not panic during normal operation

### Userspace
- [ ] bash launches and accepts commands
- [ ] vim opens, edits, and saves files
- [ ] ssh server accepts connections
- [ ] apt update and apt install work
- [ ] ping and curl work

### Tests
- [ ] Unit tests: coverage >= 70% for key modules
- [ ] All 10 system test cases pass
- [ ] GitHub Actions CI works on every PR
- [ ] README contains instructions for running and testing

---

## 9. GitHub Repository

```
README.md          — description, screenshots, quick start, architectural decisions rationale
ARCHITECTURE.md    — detailed architecture
TESTING.md         — how to run tests
CONTRIBUTING.md    — how to contribute
LICENSE            — MIT
.github/
  workflows/
    ci.yml         — GitHub Actions pipeline
```

### Commands

Both command variants are supported:

```bash
# Using full name
matryoshka build         # Build everything
matryoshka iso           # Create bootable ISO
matryoshka run           # Run in QEMU
matryoshka run-vmware    # Create VMware image
matryoshka test-unit     # Unit tests
matryoshka test-system   # System tests
matryoshka test-all      # All tests
matryoshka coverage      # Coverage report

# Using short alias (identical functionality)
mshka build
mshka iso
mshka run
mshka run-vmware
mshka test-unit
mshka test-system
mshka test-all
mshka coverage
```

---

## 10. Workflow in Cursor + Claude

Order of work at each stage:

1. Create header files `.h` — data structures and interfaces
2. Implement `.c` / `.asm`
3. Write unit tests for new module
4. `matryoshka build` (or `mshka build`) — ensure it builds without errors
5. `matryoshka run` (or `mshka run`) — check in QEMU, watch serial output
6. On kernel panic — GDB: `qemu ... -s -S` + `gdb matryoshka-kernel.elf` → `target remote :1234` → `bt`

Compilation always via **WSL2**.  
When stuck at a stage — refer to [OSDev Wiki](https://wiki.osdev.org).

### Language

The entire project is conducted in **English**:
- Code comments (`// Initialize the page table`)
- Documentation (README, ARCHITECTURE, TESTING)
- Commit messages (`feat: add round-robin scheduler`)
- Variable, function, structure names
- GitHub Issues and PR descriptions

The only exception is the original Russian specification document for internal use.

---

*MatryoshkaOS Project — System QA Portfolio*  
*Architectural decisions made with enterprise/banking environment requirements in mind*

