from PIL import Image
import sys

def main():
    if len(sys.argv) != 3:
        print("Usage: python convert_icon.py <input.png> <output.ico>")
        return
    img = Image.open(sys.argv[1])
    img.save(sys.argv[2], format='ICO', sizes=[(16,16), (32,32), (48,48), (64,64), (128,128), (256,256)])

if __name__ == '__main__':
    main()
