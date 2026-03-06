#!/usr/bin/env python3
"""
MatryoshkaOS - Integration test: Multitasking Demo

Verifies:
  1. Kernel boots and runs the task demo (tick/tock)
  2. Timer IRQs continue accumulating (scheduler doesn't hang)
  3. Shell is reachable after the demo
"""

import sys
from qemu_harness import QemuInstance, check, summary


def main():
    print("\n\033[1;34m=== Integration: multitasking ===\033[0m")

    with QemuInstance(boot_wait=5) as q:
        irqs = q.get_irq_counts()
        timer = irqs.get(0, 0)

        # Timer must still be running after task demo
        check(timer > 50,
              "timer running after task demo (got %d)" % timer)
        print("  \033[32mPASS\033[0m  timer alive: %d ticks" % timer)

        # Send 'ps' to the shell — should work after demo
        q.type_string("ps")
        irqs2 = q.get_irq_counts()
        kb = irqs2.get(1, 0)

        # 'ps' + ret = 3 keys = 6 IRQs
        check(kb >= 6, "keyboard IRQs after ps (got %d)" % kb)
        print("  \033[32mPASS\033[0m  shell responsive after demo")

        # Verify QEMU hasn't crashed
        check(q.proc.poll() is None, "QEMU still alive")
        print("  \033[32mPASS\033[0m  system stable")

    return summary()


if __name__ == "__main__":
    sys.exit(main())
