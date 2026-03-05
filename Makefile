# MatryoshkaOS Makefile
# Build system for kernel and ISO image

# Architecture
ARCH := x86_64

# Directories
KERNEL_DIR := kernel
BUILD_DIR := build
ISO_DIR := iso
ISOFILES_DIR := $(ISO_DIR)/isofiles

# Output files
KERNEL_BIN := matryoshka-kernel.elf
KERNEL_BIN_SHORT := mshka-kernel.elf
ISO_FILE := matryoshka-os.iso

# Toolchain
AS := nasm
CC := gcc
LD := ld
OBJCOPY := objcopy

# Flags
ASFLAGS := -f elf32
CFLAGS := -m32 -ffreestanding -nostdlib -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
          -Wall -Wextra -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
          -std=c11 -O2 -g -I$(KERNEL_DIR)/include
LDFLAGS := -m elf_i386 -nostdlib -T linker.ld
LIBGCC := $(shell gcc -m32 -print-libgcc-file-name)

# Source files
ASM_SOURCES := $(shell find $(KERNEL_DIR) -name '*.asm')
C_SOURCES := $(shell find $(KERNEL_DIR) -name '*.c')

# Object files
ASM_OBJECTS := $(patsubst $(KERNEL_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
C_OBJECTS := $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

# Colors for output
COLOR_RESET := \033[0m
COLOR_GREEN := \033[32m
COLOR_YELLOW := \033[33m
COLOR_BLUE := \033[34m

.PHONY: all clean iso run run-vmware test-unit test-integration test-system test-all coverage help

# Default target
all: $(BUILD_DIR)/$(KERNEL_BIN)
	@echo "$(COLOR_GREEN)✓ Build complete: $(BUILD_DIR)/$(KERNEL_BIN)$(COLOR_RESET)"

# Help target
help:
	@echo "$(COLOR_BLUE)MatryoshkaOS Build System$(COLOR_RESET)"
	@echo ""
	@echo "$(COLOR_GREEN)Available targets:$(COLOR_RESET)"
	@echo "  make all             - Build kernel (default)"
	@echo "  make clean           - Clean build artifacts"
	@echo "  make iso             - Create bootable ISO image"
	@echo "  make run             - Run in QEMU"
	@echo "  make run-vmware      - Create VMware image"
	@echo "  make test-unit       - Run unit tests"
	@echo "  make test-integration- Run integration tests"
	@echo "  make test-system     - Run system tests"
	@echo "  make test-all        - Run all tests"
	@echo "  make coverage        - Generate coverage report"
	@echo "  make help            - Show this help"

# Create build directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/arch/x86_64
	@mkdir -p $(BUILD_DIR)/mm
	@mkdir -p $(BUILD_DIR)/proc
	@mkdir -p $(BUILD_DIR)/fs
	@mkdir -p $(BUILD_DIR)/drivers
	@mkdir -p $(BUILD_DIR)/net
	@mkdir -p $(BUILD_DIR)/syscall

# Compile assembly files
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.asm | $(BUILD_DIR)
	@echo "$(COLOR_YELLOW)AS$(COLOR_RESET)   $<"
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $< -o $@

# Compile C files
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c | $(BUILD_DIR)
	@echo "$(COLOR_YELLOW)CC$(COLOR_RESET)   $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# Link kernel
$(BUILD_DIR)/$(KERNEL_BIN): $(OBJECTS)
	@echo "$(COLOR_YELLOW)LD$(COLOR_RESET)   $@"
	@$(LD) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBGCC)
	@cp $@ $(BUILD_DIR)/$(KERNEL_BIN_SHORT)
	@echo "$(COLOR_GREEN)✓ Kernel linked successfully$(COLOR_RESET)"

# Create ISO image (UEFI + Legacy BIOS hybrid)
iso: $(BUILD_DIR)/$(KERNEL_BIN)
	@echo "$(COLOR_BLUE)Creating bootable ISO image (UEFI)...$(COLOR_RESET)"
	@mkdir -p $(ISOFILES_DIR)/boot/grub
	@cp $(BUILD_DIR)/$(KERNEL_BIN) $(ISOFILES_DIR)/boot/
	@cp grub.cfg $(ISOFILES_DIR)/boot/grub/
	@grub-mkrescue --compress=xz -o $(ISO_DIR)/$(ISO_FILE) $(ISOFILES_DIR) 2>/dev/null
	@echo "$(COLOR_GREEN)✓ ISO created: $(ISO_DIR)/$(ISO_FILE)$(COLOR_RESET)"
	@echo "$(COLOR_YELLOW)  Supports: UEFI (x86_64) + Legacy BIOS (i386)$(COLOR_RESET)"

# Run in QEMU
run: iso
	@echo "$(COLOR_BLUE)Starting MatryoshkaOS in QEMU...$(COLOR_RESET)"
	@qemu-system-x86_64 \
		-cdrom $(ISO_DIR)/$(ISO_FILE) \
		-m 512M \
		-serial stdio \
		-no-reboot \
		-no-shutdown

# Run in QEMU with ISO
run-iso: iso
	@echo "$(COLOR_BLUE)Starting MatryoshkaOS ISO in QEMU...$(COLOR_RESET)"
	@qemu-system-x86_64 \
		-cdrom $(ISO_DIR)/$(ISO_FILE) \
		-m 512M \
		-serial stdio \
		-no-reboot \
		-no-shutdown

# Debug with GDB
debug: $(BUILD_DIR)/$(KERNEL_BIN)
	@echo "$(COLOR_BLUE)Starting QEMU with GDB stub (port 1234)...$(COLOR_RESET)"
	@qemu-system-x86_64 \
		-kernel $(BUILD_DIR)/$(KERNEL_BIN) \
		-m 512M \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-s -S &
	@echo "$(COLOR_YELLOW)Run: gdb $(BUILD_DIR)/$(KERNEL_BIN) -ex 'target remote :1234'$(COLOR_RESET)"

# VMware image
run-vmware:
	@echo "$(COLOR_YELLOW)VMware support coming soon...$(COLOR_RESET)"
	@echo "For now, use the ISO: $(ISO_DIR)/$(ISO_FILE)"

# Unit tests
test-unit:
	@echo "$(COLOR_BLUE)Running unit tests...$(COLOR_RESET)"
	@cd tests/unit && make test

# Integration tests
test-integration:
	@echo "$(COLOR_BLUE)Running integration tests...$(COLOR_RESET)"
	@cd tests/integration && make test

# System tests
test-system:
	@echo "$(COLOR_BLUE)Running system tests...$(COLOR_RESET)"
	@cd tests/system && pytest -v

# All tests
test-all: test-unit test-integration test-system
	@echo "$(COLOR_GREEN)✓ All tests completed$(COLOR_RESET)"

# Coverage report
coverage:
	@echo "$(COLOR_BLUE)Generating coverage report...$(COLOR_RESET)"
	@gcovr -r . --html --html-details -o coverage.html
	@echo "$(COLOR_GREEN)✓ Coverage report: coverage.html$(COLOR_RESET)"

# Clean build artifacts
clean:
	@echo "$(COLOR_YELLOW)Cleaning build artifacts...$(COLOR_RESET)"
	@rm -rf $(BUILD_DIR)
	@rm -rf $(ISO_DIR)/isofiles
	@rm -f $(ISO_DIR)/*.iso
	@echo "$(COLOR_GREEN)✓ Clean complete$(COLOR_RESET)"

# Show kernel info
info: $(BUILD_DIR)/$(KERNEL_BIN)
	@echo "$(COLOR_BLUE)Kernel Information:$(COLOR_RESET)"
	@file $(BUILD_DIR)/$(KERNEL_BIN)
	@size $(BUILD_DIR)/$(KERNEL_BIN)
	@echo ""
	@echo "$(COLOR_BLUE)Object files:$(COLOR_RESET)"
	@ls -lh $(BUILD_DIR)/*.o 2>/dev/null || true

