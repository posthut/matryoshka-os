# MatryoshkaOS

A minimal Linux-compatible operating system built from scratch for learning system-level QA engineering.

[![CI](https://github.com/YOUR_USERNAME/matryoshka-os/workflows/MatryoshkaOS%20CI/badge.svg)](https://github.com/YOUR_USERNAME/matryoshka-os/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Stage](https://img.shields.io/badge/stage-2%2F10-yellow)](PROJECT_STATUS.md)
[![Lines of Code](https://img.shields.io/badge/lines-~600-green)]()
[![Platform](https://img.shields.io/badge/platform-x86__64-orange)]()

> **Note:** Replace `YOUR_USERNAME` in the CI badge with your actual GitHub username after pushing to GitHub.

## 🎯 Project Goals

1. **Architecturally clean kernel** written in C and x86_64 Assembly from scratch
2. **Professional testing infrastructure** for system-level quality assurance
3. **Portfolio project** for System QA Engineer position

## 🏗️ Architecture

```
┌─────────────────────────────────────────┐
│  Userspace: bash, vim, ssh, apt         │  ← Debian base
├─────────────────────────────────────────┤
│  Init (PID 1)                           │  ← Custom implementation
├─────────────────────────────────────────┤
│  Syscall Interface (Linux-compatible)   │  ← Custom implementation
├─────────────────────────────────────────┤
│  Kernel: mm, proc, fs, drivers, net     │  ← Custom implementation
├─────────────────────────────────────────┤
│  Bootloader: GRUB2                      │  ← Industry standard
└─────────────────────────────────────────┘
```

**Key Decision:** The kernel implements Linux-compatible syscall ABI, allowing Debian binaries (vim, ssh, apt) to run without modifications.

## 🚀 Quick Start

### Prerequisites

**Host System:**
- Windows 11 with WSL2 (Ubuntu)
- VMware Workstation Pro or QEMU

**In WSL2:**
```bash
sudo apt update
sudo apt install -y \
    gcc-x86-64-linux-gnu \
    nasm \
    binutils \
    grub-pc-bin \
    grub-common \
    xorriso \
    qemu-system-x86 \
    python3 \
    python3-pytest \
    python3-pexpect \
    debootstrap \
    gcovr \
    lcov
```

### Build and Run

```bash
# Build kernel (both commands are equivalent)
matryoshka build
# or
mshka build

# Create bootable ISO
matryoshka iso

# Run in QEMU
matryoshka run

# Run all tests
matryoshka test-all

# Generate coverage report
matryoshka coverage
```

## 📁 Repository Structure

```
matryoshka-os/
├── kernel/              # Kernel source code
│   ├── arch/x86_64/    # Architecture-specific (ASM, GDT, IDT)
│   ├── mm/             # Memory management
│   ├── proc/           # Process management
│   ├── fs/             # Filesystem (VFS, ext2)
│   ├── drivers/        # Device drivers
│   ├── net/            # Network stack
│   └── syscall/        # System call interface
├── userspace/          # Init and userspace programs
├── rootfs/             # Debian rootfs build scripts
├── tests/              # Testing infrastructure
│   ├── unit/          # Unity C unit tests
│   ├── integration/   # Integration tests
│   ├── system/        # End-to-end tests (pytest)
│   └── regression/    # CI smoke tests
├── tools/              # Helper scripts
├── docs/               # Documentation
└── scripts/            # Build and automation scripts
```

## 🧪 Testing

MatryoshkaOS includes comprehensive testing infrastructure:

- **Unit Tests**: `matryoshka test-unit` — Test individual kernel modules
- **Integration Tests**: `matryoshka test-integration` — Test module interactions
- **System Tests**: `matryoshka test-system` — End-to-end testing with pytest
- **Coverage**: Goal >= 70% for critical kernel modules

## 📚 Documentation

- [Quick Start](docs/QUICKSTART.md) — Get running in 5 minutes
- [Build Guide](docs/BUILD.md) — Detailed build instructions
- [Git Setup](docs/GIT_SETUP.md) — Initialize repository and push to GitHub
- [Technical Specification](TECHNICAL_SPECIFICATION.md) — Complete technical design
- [Development Rules](DEVELOPMENT_RULES.md) — Coding standards and practices
- [Contributing](CONTRIBUTING.md) — How to contribute to the project
- [Code of Conduct](CODE_OF_CONDUCT.md) — Community guidelines
- [Security Policy](SECURITY.md) — Security considerations
- [Project Status](PROJECT_STATUS.md) — Current development status

## 🛠️ Development Workflow

1. Create header files (`.h`) — data structures and interfaces
2. Implement (`.c` / `.asm`)
3. Write unit tests
4. Build and verify compilation
5. Test in QEMU
6. Debug with GDB if needed

### Why GRUB2?

GRUB2 is used as the bootloader for solid architectural reasons:
- **Security**: 20+ years of public auditing and vulnerability management
- **Secure Boot**: UEFI Secure Boot support required in enterprise environments
- **Regulatory Compliance**: Meets banking sector requirements (PCI DSS, Central Bank)
- **Resource Focus**: Development resources directed to kernel and testing infrastructure

This follows industry best practices used by enterprise Linux distributions.

## 📊 Current Status

- [x] Project structure
- [x] Minimal kernel (Hello World)
- [ ] Memory management (PMM, VMM, heap)
- [ ] Interrupt handling (GDT, IDT, PIC)
- [ ] Process management (scheduler, fork, exec)
- [ ] Filesystem (VFS, ext2)
- [ ] System calls (Linux-compatible ABI)
- [ ] Network stack (e1000, TCP/IP)
- [ ] Userspace (init, Debian rootfs)
- [ ] Testing infrastructure

## 🤝 Contributing

This is an educational project built for portfolio purposes. Contributions, suggestions, and feedback are welcome!

See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed guidelines on:
- Development workflow
- Coding standards
- Commit message format
- Pull request process
- Testing requirements

Please read our [Code of Conduct](CODE_OF_CONDUCT.md) before contributing.

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

## 🎓 Learning Resources

- [OSDev Wiki](https://wiki.osdev.org) — Primary resource for OS development
- [Intel Software Developer Manuals](https://software.intel.com/content/www/us/en/develop/articles/intel-sdm.html)
- [AMD64 Architecture Programmer's Manual](https://www.amd.com/en/support/tech-docs)

---

**MatryoshkaOS** — Built with enterprise-grade architecture decisions for learning system-level QA engineering.

