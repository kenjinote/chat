import os

def create_ico(png_files, output_file):
    num_images = len(png_files)
    header = b'\x00\x00\x01\x00' + num_images.to_bytes(2, 'little')
    
    # Read all pngs
    png_data = []
    for f in png_files:
        with open(f, 'rb') as fp:
            png_data.append(fp.read())
            
    # Calculate offsets
    offset = 6 + 16 * num_images
    directory = bytearray()
    
    for i, data in enumerate(png_data):
        # We need width and height. For square PNGs, we can extract it or just hardcode based on the file.
        # But we know the files: tmp16, tmp24, etc.
        size = int(png_files[i].replace('tmp', '').replace('.png', ''))
        w = size if size < 256 else 0
        h = size if size < 256 else 0
        
        directory += w.to_bytes(1, 'little')
        directory += h.to_bytes(1, 'little')
        directory += b'\x10' # 16 colors
        directory += b'\x00' # reserved
        directory += b'\x01\x00' # planes
        directory += b'\x04\x00' # bit count (4-bit)
        directory += len(data).to_bytes(4, 'little')
        directory += offset.to_bytes(4, 'little')
        
        offset += len(data)
        
    with open(output_file, 'wb') as f:
        f.write(header)
        f.write(directory)
        for data in png_data:
            f.write(data)
            
    print(f"Created {output_file} with size {os.path.getsize(output_file)} bytes")

pngs = ["tmp16.png", "tmp24.png", "tmp32.png", "tmp48.png", "tmp64.png", "tmp128.png", "tmp256.png"]
create_ico(pngs, "app.ico")
