from PIL import Image
import sys

def make_transparent(input_path, output_path):
    img = Image.open(input_path).convert("RGBA")
    datas = img.getdata()
    
    newData = []
    for item in datas:
        # change all white (also shades of white) to transparent
        if item[0] > 240 and item[1] > 240 and item[2] > 240:
            newData.append((255, 255, 255, 0))
        else:
            newData.append(item)
            
    img.putdata(newData)
    img.save(output_path, format='ICO', sizes=[(16,16), (32,32), (48,48), (64,64), (128,128), (256,256)])

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python make_transparent.py <input.png> <output.ico>")
        sys.exit(1)
    make_transparent(sys.argv[1], sys.argv[2])
