from PIL import Image

def build_ico():
    img = Image.open("origin_bevel.png").convert("RGBA")
    
    sizes = [16, 24, 32, 48, 64, 128, 256]
    img_list = []
    
    for size in sizes:
        resized = img.resize((size, size), Image.NEAREST)
        quantized = resized.quantize(colors=16)
        img_list.append(quantized)
        
    img_list[0].save(
        "app.ico",
        format="ICO",
        append_images=img_list[1:]
    )
    print("Saved app.ico using append_images only.")

if __name__ == "__main__":
    build_ico()
