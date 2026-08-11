import os
from PIL import Image

ico_paths = [
    r'C:\Users\kenjinote\Desktop\chat\app.ico',
    r'C:\Users\kenjinote\Desktop\chat\tv.ico'
]

for ico in ico_paths:
    if os.path.exists(ico):
        img = Image.open(ico)
        png_path = ico.replace('.ico', '.png')
        img.save(png_path)
        print(f"Converted {ico} to {png_path}")
    else:
        print(f"Not found: {ico}")
