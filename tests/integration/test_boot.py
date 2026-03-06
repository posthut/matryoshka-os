#!/usr/bin/env python3
"""
MatryoshkaOS - Integration test: Kernel Boot & Interrupts

Verifies:
  1. Kernel boots without triple-fault
  2. Timer interrupts fire (IRQ0 count > 0)
  3. QEMU stays alive for the full boot window
"""

import sys
from qemu_harness import QemuInstance, check, summary


def main():
    print("\n\033[1;34m=== Integration: boot & interrupts ===\033[0m")

    with QemuInstance(boot_wait=4) as q:
        irqs = q.get_irq_counts()

        # Timer must be firing
        timer_count = irqs.get(0, 0)
        check(timer_count > 50, "timer IRQ0 count > 50 (got %d)" % timer_count)
        print("  \033[32mPASS\033[0m  timer ticking (%d IRQs)" % timer_count)

        # QEMU process should still be running (no triple-fault)
        check(q.proc.poll() is None, "QEMU still running")
        print("  \033[32mPASS\033[0m  kernel alive (no triple fault)")

    return summary()


if __name__ == "__main__":
    sys.exit(main())
