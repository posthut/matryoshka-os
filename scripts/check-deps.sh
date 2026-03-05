#!/bin/bash
# MatryoshkaOS - Check Build Dependencies
# Verifies all required tools are installed in WSL2

echo "🔍 Checking MatryoshkaOS Build Dependencies"
echo "============================================"
echo ""

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

missing_packages=()
all_ok=true

# Check function
check_tool() {
    local tool=$1
    local package=$2
    local min_version=$3
    
    if command -v "$tool" &> /dev/null; then
        version=$("$tool" --version 2>&1 | head -n 1)
        echo -e "${GREEN}✓${NC} $tool: $version"
    else
        echo -e "${RED}✗${NC} $tool: NOT FOUND"
        if [ -n "$package" ]; then
            missing_packages+=("$package")
        fi
        all_ok=false
    fi
}

echo "📦 Core Build Tools:"
check_tool "make" "make"
check_tool "gcc" "build-essential"

echo ""
echo "🔧 Cross-Compiler Tools:"
check_tool "x86_64-linux-gnu-gcc" "gcc-x86-64-linux-gnu"
check_tool "x86_64-linux-gnu-ld" "binutils"
check_tool "nasm" "nasm"

echo ""
echo "💿 Bootloader Tools:"
check_tool "grub-mkrescue" "grub-pc-bin grub-common"
check_tool "xorriso" "xorriso"

echo ""
echo "🖥️  Emulation:"
check_tool "qemu-system-x86_64" "qemu-system-x86"

echo ""
echo "🧪 Testing Tools:"
check_tool "python3" "python3"
check_tool "pytest" "python3-pytest"

echo ""
echo "📊 Coverage Tools:"
check_tool "gcov" "gcc"
check_tool "lcov" "lcov"

echo ""
echo "📁 Other Tools:"
check_tool "git" "git"
check_tool "debootstrap" "debootstrap"

echo ""
echo "============================================"

if [ "$all_ok" = true ]; then
    echo -e "${GREEN}✅ All dependencies are installed!${NC}"
    echo ""
    echo "You can now build MatryoshkaOS:"
    echo "  cd /mnt/c/myDesc/devSpace/matryoshka-os"
    echo "  make clean"
    echo "  make all"
    echo "  make run"
else
    echo -e "${RED}❌ Missing dependencies detected${NC}"
    echo ""
    echo "Install missing packages:"
    echo ""
    
    # Remove duplicates
    unique_packages=($(echo "${missing_packages[@]}" | tr ' ' '\n' | sort -u | tr '\n' ' '))
    
    echo "  sudo apt update"
    echo "  sudo apt install -y ${unique_packages[@]}"
    echo ""
    echo "Or install all at once:"
    echo ""
    echo "  sudo apt update && sudo apt install -y \\"
    echo "    gcc \\"
    echo "    make \\"
    echo "    gcc-x86-64-linux-gnu \\"
    echo "    nasm \\"
    echo "    binutils \\"
    echo "    grub-pc-bin \\"
    echo "    grub-common \\"
    echo "    xorriso \\"
    echo "    qemu-system-x86 \\"
    echo "    python3 \\"
    echo "    python3-pytest \\"
    echo "    python3-pexpect \\"
    echo "    lcov \\"
    echo "    gcovr \\"
    echo "    git \\"
    echo "    debootstrap"
fi

echo ""

