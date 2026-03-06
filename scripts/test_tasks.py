#!/usr/bin/env python3
"""Boot kernel, wait for task demo, then send shell commands via QEMU monitor."""
import socket, time, subprocess, os, signal

QEMU_MON = '/tmp/qemu-task-test.sock'

# Kill any leftover QEMU
os.system('pkill -f "qemu-system-i386.*matryoshka" 2>/dev/null')
time.sleep(0.3)

# Remove stale socket
if os.path.exists(QEMU_MON):
    os.unlink(QEMU_MON)

# Start QEMU in background
qemu = subprocess.Popen([
    'qemu-system-i386',
    '-cdrom', 'iso/matryoshka-os.iso',
    '-m', '128M',
    '-no-reboot',
    '-display', 'none',
    '-serial', 'file:/tmp/matryoshka-serial.log',
    '-monitor', 'unix:%s,server,nowait' % QEMU_MON,
], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

print("[*] QEMU started, pid=%d" % qemu.pid)

# Wait for boot + task demo (~4 sec)
time.sleep(4)

# Connect to monitor
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
s.settimeout(3)
try:
    s.connect(QEMU_MON)
    banner = s.recv(4096)
    print("[*] Monitor connected")
except Exception as e:
    print("[!] Failed to connect: %s" % e)
    qemu.terminate()
    exit(1)

def sendkey(key):
    s.send(('sendkey %s\n' % key).encode())
    time.sleep(0.12)

def send_string(text):
    for ch in text:
        sendkey(ch)
    sendkey('ret')

def monitor_cmd(cmd):
    s.send((cmd + '\n').encode())
    time.sleep(0.4)
    try:
        return s.recv(8192).decode(errors='replace')
    except:
        return ''

# Send 'ps' command
print("[*] Sending 'ps' command")
send_string('ps')
time.sleep(1)

# Send 'uptime' command
print("[*] Sending 'uptime' command")
send_string('uptime')
time.sleep(1)

# Check IRQ stats
irq_data = monitor_cmd('info irq')
print("[*] IRQ stats:")
print(irq_data)

# Take screendump
monitor_cmd('screendump /tmp/matryoshka-screen.ppm')
time.sleep(0.3)

# Cleanup
s.close()
qemu.terminate()
qemu.wait()
print("[*] QEMU terminated")

# Show screenshot info
if os.path.exists('/tmp/matryoshka-screen.ppm'):
    sz = os.path.getsize('/tmp/matryoshka-screen.ppm')
    print("[*] Screenshot saved: /tmp/matryoshka-screen.ppm (%d bytes)" % sz)
