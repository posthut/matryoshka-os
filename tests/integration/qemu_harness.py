"""
MatryoshkaOS - QEMU test harness.

Boots the kernel in QEMU, connects to the monitor socket, and provides
helpers for sending keystrokes / reading IRQ stats / taking screenshots.
"""

import os
import socket
import subprocess
import sys
import time

QEMU_BIN  = "qemu-system-i386"
ISO_PATH  = os.path.join(os.path.dirname(__file__), "..", "..", "iso", "matryoshka-os.iso")
MON_SOCK  = "/tmp/mshka-test-mon.sock"
SCREEN    = "/tmp/mshka-test-screen.ppm"
BOOT_WAIT = 3.5          # seconds for kernel to boot
KEY_DELAY = 0.12          # seconds between sendkey commands


class QemuInstance:
    """Context manager that starts QEMU and connects to its monitor."""

    def __init__(self, boot_wait=BOOT_WAIT):
        self.proc = None
        self.sock = None
        self.boot_wait = boot_wait

    def __enter__(self):
        self._cleanup_stale()
        self.proc = subprocess.Popen([
            QEMU_BIN,
            "-cdrom", ISO_PATH,
            "-m", "128M",
            "-no-reboot",
            "-display", "none",
            "-monitor", "unix:%s,server,nowait" % MON_SOCK,
        ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        time.sleep(self.boot_wait)

        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.settimeout(3)
        self.sock.connect(MON_SOCK)
        self.sock.recv(4096)   # consume banner
        time.sleep(0.2)
        return self

    def __exit__(self, *exc):
        if self.sock:
            try:
                self.sock.close()
            except Exception:
                pass
        if self.proc:
            self.proc.terminate()
            self.proc.wait()
        self._cleanup_stale()

    def _cleanup_stale(self):
        os.system('pkill -f "qemu-system.*mshka-test" 2>/dev/null')
        for p in (MON_SOCK, SCREEN):
            if os.path.exists(p):
                os.unlink(p)

    # ── Monitor helpers ──────────────────────────────────────────

    def monitor_cmd(self, cmd):
        self.sock.send((cmd + "\n").encode())
        time.sleep(0.4)
        try:
            return self.sock.recv(8192).decode(errors="replace")
        except socket.timeout:
            return ""

    def sendkey(self, key):
        self.sock.send(("sendkey %s\n" % key).encode())
        time.sleep(KEY_DELAY)

    def type_string(self, text):
        for ch in text:
            self.sendkey(ch)
        self.sendkey("ret")
        time.sleep(0.5)

    def get_irq_counts(self):
        """Return dict {irq_num: count} from 'info irq'."""
        raw = self.monitor_cmd("info irq")
        counts = {}
        for line in raw.splitlines():
            line = line.strip()
            if line and line[0].isdigit():
                parts = line.split(":")
                if len(parts) == 2:
                    try:
                        counts[int(parts[0].strip())] = int(parts[1].strip())
                    except ValueError:
                        pass
        return counts

    def screenshot(self, path=SCREEN):
        self.monitor_cmd("screendump %s" % path)
        time.sleep(0.3)
        return path


# ── Lightweight assertion helpers (no pytest needed) ──────────────

_pass = _fail = 0


def check(expr, msg=""):
    global _pass, _fail
    if expr:
        _pass += 1
    else:
        _fail += 1
        frame = sys._getframe(1)
        loc = "%s:%d" % (frame.f_code.co_filename, frame.f_lineno)
        print("  \033[31mFAIL\033[0m  %s  %s" % (loc, msg))


def summary():
    total = _pass + _fail
    color = "\033[32m" if _fail == 0 else "\033[31m"
    print("\n%s--- %d/%d checks passed\033[0m" % (color, _pass, total))
    return 0 if _fail == 0 else 1
