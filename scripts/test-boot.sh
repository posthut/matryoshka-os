#!/bin/bash
# MatryoshkaOS - Quick Test Script
# Builds and runs in QEMU with serial output capture

cd /mnt/c/myDesc/devSpace/matryoshka-os

echo "🔨 Building kernel..."
make clean > /dev/null 2>&1
make all 2>&1 | grep -E "✓|error|warning" || true

echo ""
echo "💿 Creating ISO..."
make iso 2>&1 | grep -E "✓|error" || true

echo ""
echo "🚀 Running in QEMU (15 seconds)..."
echo "   Press Ctrl+A then X to exit QEMU manually"
echo ""

timeout 15s qemu-system-x86_64 \
    -cdrom iso/matryoshka-os.iso \
    -m 512M \
    -serial file:serial.log \
    -display none \
    -no-reboot \
    -no-shutdown 2>&1 || true

echo ""
echo "📋 Serial output:"
echo "================="
if [ -f serial.log ]; then
    cat serial.log
    echo ""
    echo "================="
    
    # Check for success indicators
    if grep -q "MatryoshkaOS" serial.log; then
        echo "✅ Kernel booted!"
    fi
    
    if grep -q "Physical Memory Manager" serial.log; then
        echo "✅ PMM initialized!"
    fi
    
    if grep -q "Testing PMM" serial.log; then
        echo "✅ PMM tests ran!"
    fi
else
    echo "❌ No serial output captured"
fi

