#!/bin/bash
# MatryoshkaOS - Build Script for Windows
# Builds kernel in WSL2 and runs in QEMU

set -e

cd /mnt/c/myDesc/devSpace/matryoshka-os

echo "🔨 Building MatryoshkaOS..."
make clean
make all

echo ""
echo "✅ Build complete!"
echo ""
echo "📊 Kernel info:"
file build/matryoshka-kernel.elf
size build/matryoshka-kernel.elf

echo ""
echo "🚀 To run: make run"

