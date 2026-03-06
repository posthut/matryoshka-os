#!/usr/bin/env python3
"""
MatryoshkaOS - Integration test: Shell Commands

Verifies that typing commands into the kernel shell produces
the expected keyboard IRQs (correct key count).
"""

import sys
from qemu_harness import QemuInstance, check, summary


def main():
    print("\n\033[1;34m=== Integration: shell commands ===\033[0m")

    with QemuInstance(boot_wait=4) as q:
        irqs_before = q.get_irq_counts()
        kb_before = irqs_before.get(1, 0)

        # Type 'help' (4 letters + ret = 5 keys = 10 IRQs: make + break)
        q.type_string("help")
        irqs_after = q.get_irq_counts()
        kb_after = irqs_after.get(1, 0)
        kb_delta = kb_after - kb_before

        check(kb_delta == 10,
              "'help'+ret = 5 keys = 10 IRQs (got %d)" % kb_delta)
        print("  \033[32mPASS\033[0m  help command: %d keyboard IRQs" % kb_delta)

        # Type 'uptime' (6 letters + ret = 7 keys = 14 IRQs)
        kb_before = kb_after
        q.type_string("uptime")
        irqs_after = q.get_irq_counts()
        kb_after = irqs_after.get(1, 0)
        kb_delta = kb_after - kb_before

        check(kb_delta == 14,
              "'uptime'+ret = 7 keys = 14 IRQs (got %d)" % kb_delta)
        print("  \033[32mPASS\033[0m  uptime command: %d keyboard IRQs" % kb_delta)

        # Type 'ps' (2 letters + ret = 3 keys = 6 IRQs)
        kb_before = kb_after
        q.type_string("ps")
        irqs_after = q.get_irq_counts()
        kb_after = irqs_after.get(1, 0)
        kb_delta = kb_after - kb_before

        check(kb_delta == 6,
              "'ps'+ret = 3 keys = 6 IRQs (got %d)" % kb_delta)
        print("  \033[32mPASS\033[0m  ps command: %d keyboard IRQs" % kb_delta)

    return summary()


if __name__ == "__main__":
    sys.exit(main())
