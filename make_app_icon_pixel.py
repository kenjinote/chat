from PIL import Image

grid = [
"BBBBBBBBBBBBBBBB",
"BWWWWWWWWWWWWWWB",
"BWWWWWWWWWWWWWWB",
"BWWWWWWWWWWWWWWB",
"BWWWWWWWWWWWWWWB",
"BWWWWWWWWWWWWWWB",
"BWWWWBWBWBWWWWWB",
"BWWWWWWWWWWWWWWB",
"BWWWWWWWWWWWWWWB",
"BWWWWWWWWWWWWWWB",
"BWWWWWWWWWWWWWWB",
"BWWWWWWWWWWWWWWB",
"BBBBWWBBBBBBBBBB",
"    BWB         ",
"    BWB         ",
"     BB         ",
]

colors = {
    ' ': (0, 0, 0, 0),
    'B': (0, 0, 0, 255),
    'W': (255, 255, 255, 255)
}

img16 = Image.new('RGBA', (16, 16))
pixels = img16.load()

for y in range(16):
    for x in range(16):
        pixels[x, y] = colors[grid[y][x]]

sizes = [16, 24, 32, 48, 64, 128, 256]
imgs = []
for s in sizes:
    imgs.append(img16.resize((s, s), Image.NEAREST))

import struct
import io
out = open('app.ico', 'wb')
out.write(struct.pack('<HHH', 0, 1, len(imgs)))

offset = 6 + 16 * len(imgs)
png_data = []

for img in imgs:
    b = io.BytesIO()
    img.save(b, format='PNG')
    data = b.getvalue()
    png_data.append(data)
    w = img.width if img.width < 256 else 0
    h = img.height if img.height < 256 else 0
    out.write(struct.pack('<BBBBHHII', w, h, 0, 0, 1, 32, len(data), offset))
    offset += len(data)

for data in png_data:
    out.write(data)

out.close()
print('Pixel art app.ico generated with explicit multi-size PNG frames.')

