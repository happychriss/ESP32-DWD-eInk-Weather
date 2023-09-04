import os
from PIL import Image
import math

SCREEN_WIDTH = 1200
SCREEN_HEIGHT = 825

if SCREEN_WIDTH % 2:
    print("image width must be even!")
    exit(1)


def process_image(image_path, const_name, output_file):
    im = Image.open(image_path)
    bg = Image.new("RGB", im.size, (255, 255, 255))
    bg.paste(im, im)
    im = bg
    # convert to grayscale
    im = im.convert(mode='L')

    output_file.write("const uint32_t {}_width = {};\n".format(const_name, im.size[0]))
    output_file.write("const uint32_t {}_height = {};\n".format(const_name, im.size[1]))
    output_file.write(
        "const uint8_t {}_data[({}*{})/2] PROGMEM = {{\n\t".format(const_name, math.ceil(im.size[0] / 2) * 2, im.size[1])
    )
    for y in range(0, im.size[1]):
        byte = 0
        done = True
        for x in range(0, im.size[0]):
            l = im.getpixel((x, y))
            if x % 2 == 0:
                byte = l >> 4
                done = False
            else:
                byte |= l & 0xF0
                output_file.write("0x{:02X}, ".format(byte))
                done = True
        if not done:
            output_file.write("0x{:02X}, ".format(byte))
        output_file.write("\n\t")
    output_file.write("};\n\n")


if __name__ == "__main__":
    folder_path = "/home/development/Projects/Experiements/LilyGo_Weather/weather_icons"  # Update this path
    output_filename = "src/image/output.h"

    with open(output_filename, 'w') as output_file:
        for filename in os.listdir(folder_path):
            if filename.endswith(".jpg") or filename.endswith(".png"):
                image_path = os.path.join(folder_path, filename)
                const_name = os.path.splitext(filename)[0]
                process_image(image_path, const_name, output_file)
