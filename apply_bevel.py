from PIL import Image

def apply_win95_bevel(input_path, output_path):
    img = Image.open(input_path).convert("RGBA")
    pixels = img.load()
    width, height = img.size

    # Create a new image for the output
    out_img = Image.new("RGBA", (width, height))
    out_pixels = out_img.load()

    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            
            if a > 128:  # Solid pixel
                # Check neighbors to see if it's an edge
                top = pixels[x, y-1][3] if y > 0 else 0
                bottom = pixels[x, y+1][3] if y < height - 1 else 0
                left = pixels[x-1, y][3] if x > 0 else 0
                right = pixels[x+1, y][3] if x < width - 1 else 0
                
                is_top_left_edge = (top <= 128 or left <= 128)
                is_bottom_right_edge = (bottom <= 128 or right <= 128)
                
                if is_top_left_edge and not is_bottom_right_edge:
                    # Highlight (White)
                    out_pixels[x, y] = (255, 255, 255, a)
                elif is_bottom_right_edge and not is_top_left_edge:
                    # Shadow (Black)
                    out_pixels[x, y] = (0, 0, 0, a)
                elif is_top_left_edge and is_bottom_right_edge:
                    # Single pixel wide parts
                    if r < 128 and g < 128 and b < 128:
                        out_pixels[x, y] = (r, g, b, a)
                    else:
                        out_pixels[x, y] = (192, 192, 192, a)
                else:
                    # Inner pixel
                    if r < 128 and g < 128 and b < 128:
                        out_pixels[x, y] = (r, g, b, a)
                    else:
                        out_pixels[x, y] = (192, 192, 192, a)
            else:
                # Transparent pixel
                out_pixels[x, y] = (r, g, b, a)
                
    out_img.save(output_path)
    print(f"Saved {output_path}")

if __name__ == "__main__":
    apply_win95_bevel("origin.png", "origin_bevel.png")
