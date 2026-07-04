from PIL import Image, ImageDraw

def draw_tv(size):
    img = Image.new('RGBA', (size, size), (255, 255, 255, 0))
    draw = ImageDraw.Draw(img)
    
    # Scale factors
    scale = size / 32.0
    
    # Coordinates
    cabinet_x = 2 * scale
    cabinet_y = 6 * scale
    cabinet_w = 28 * scale
    cabinet_h = 24 * scale
    
    # Cabinet
    draw.rectangle([cabinet_x, cabinet_y, cabinet_x + cabinet_w, cabinet_y + cabinet_h], fill=(139, 69, 19, 255), outline=(101, 67, 33, 255))
    
    # Bezel
    bezel_x = cabinet_x + 2 * scale
    bezel_y = cabinet_y + 2 * scale
    bezel_w = 20 * scale
    bezel_h = 20 * scale
    draw.rectangle([bezel_x, bezel_y, bezel_x + bezel_w, bezel_y + bezel_h], fill=(50, 50, 50, 255))
    
    # Screen
    screen_x = bezel_x + 2 * scale
    screen_y = bezel_y + 2 * scale
    screen_w = 16 * scale
    screen_h = 16 * scale
    draw.rectangle([screen_x, screen_y, screen_x + screen_w, screen_y + screen_h], fill=(180, 190, 200, 255))
    
    # Knobs
    knob_x = cabinet_x + 24 * scale
    draw.ellipse([knob_x, cabinet_y + 4 * scale, knob_x + 2 * scale, cabinet_y + 6 * scale], fill=(20, 20, 20, 255))
    draw.ellipse([knob_x, cabinet_y + 10 * scale, knob_x + 2 * scale, cabinet_y + 12 * scale], fill=(20, 20, 20, 255))
    
    # Speaker lines
    draw.line([knob_x, cabinet_y + 16 * scale, knob_x + 2 * scale, cabinet_y + 16 * scale], fill=(60, 60, 60, 255))
    draw.line([knob_x, cabinet_y + 18 * scale, knob_x + 2 * scale, cabinet_y + 18 * scale], fill=(60, 60, 60, 255))
    draw.line([knob_x, cabinet_y + 20 * scale, knob_x + 2 * scale, cabinet_y + 20 * scale], fill=(60, 60, 60, 255))
    
    # Antenna
    ant_base_x = cabinet_x + cabinet_w / 2
    ant_base_y = cabinet_y
    draw.line([ant_base_x, ant_base_y, ant_base_x - 6 * scale, ant_base_y - 6 * scale], fill=(150, 150, 150, 255), width=int(max(1, scale)))
    draw.line([ant_base_x, ant_base_y, ant_base_x + 6 * scale, ant_base_y - 6 * scale], fill=(150, 150, 150, 255), width=int(max(1, scale)))
    
    # Antenna tips
    draw.ellipse([ant_base_x - 8 * scale, ant_base_y - 8 * scale, ant_base_x - 4 * scale, ant_base_y - 4 * scale], fill=(200, 50, 50, 255))
    draw.ellipse([ant_base_x + 4 * scale, ant_base_y - 8 * scale, ant_base_x + 8 * scale, ant_base_y - 4 * scale], fill=(200, 50, 50, 255))
    
    return img

images = []
for sz in [16, 32, 48, 64, 128, 256]:
    images.append(draw_tv(sz))

images[0].save('tv.ico', format='ICO', sizes=[(16,16), (32,32), (48,48), (64,64), (128,128), (256,256)], append_images=images[1:])
print("tv.ico created successfully.")
