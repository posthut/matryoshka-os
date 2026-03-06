"""Convert PPM to PNG using pure Python (no PIL needed)."""
import struct

with open('/tmp/matryoshka-screen.ppm', 'rb') as f:
    magic = f.readline().strip()
    assert magic == b'P6', 'Not a P6 PPM'
    line = f.readline().strip()
    while line.startswith(b'#'):
        line = f.readline().strip()
    w, h = map(int, line.split())
    maxval = int(f.readline().strip())
    data = f.read()

import zlib

def make_png(width, height, rgb_data):
    def chunk(ctype, cdata):
        c = ctype + cdata
        return struct.pack('>I', len(cdata)) + c + struct.pack('>I', zlib.crc32(c) & 0xFFFFFFFF)

    raw = b''
    for y in range(height):
        raw += b'\x00'
        raw += rgb_data[y * width * 3:(y + 1) * width * 3]

    png = b'\x89PNG\r\n\x1a\n'
    png += chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(raw))
    png += chunk(b'IEND', b'')
    return png

png_data = make_png(w, h, data)
with open('/tmp/matryoshka-screen.png', 'wb') as f:
    f.write(png_data)

print('Saved PNG: %dx%d (%d bytes)' % (w, h, len(png_data)))
