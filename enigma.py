#!/usr/bin/env python3

import argparse
import os
from pathlib import Path
import xml.etree.ElementTree as ET


class DimensionAction(argparse.Action):
    def __init__(self, option_strings, dest, nargs=None, **kwargs):
        if nargs is not None:
            raise ValueError("nargs not allowed")
        super().__init__(option_strings, dest, **kwargs)

    def __call__(self, parser, namespace, values, option_string=None):
        raw = values.split("x")
        dimension = {
            "width": int(raw[0]),
            "height": int(raw[1]),
        }
        setattr(namespace, self.dest, dimension)


parser = argparse.ArgumentParser("tiled2zeal")
parser.add_argument("-i", "--input", help="Input TMX Filename", required=True)
parser.add_argument("-o", "--output", help="Output path, can be just a path")
parser.add_argument(
    "-s",
    "--size",
    help="Screen Width/Height, for larger world maps",
    action=DimensionAction,
    default=None,
)
parser.add_argument(
    "-z", "--compress", help="Compress ZTM with RLE", action="store_true"
)
parser.add_argument("-v", "--verbose", help="Verbose output", action="store_true")
parser.add_argument("-d", "--debug", help="Debug output", action="store_true")

root = None
meta = None

# starting index of the gate numbers
FUEL_INDEX = 5
GATE_INDEX = 80
GATE_COUNT = 9
KEY_INDEX = 96
WORLD_WIDTH = 8
WORLD_HEIGHT = 8

HEADER_COLOR = "\033[36m"  # Cyan
SEPARATOR_COLOR = "\033[33m"  # Yellow
ROW_INDEX_COLOR = "\033[35m"  # Magenta
DATA_COLOR = "\033[32m"  # Green
RESET = "\033[0m"  # Reset to default


def create_dir(file_path):
    if "." in os.path.basename(file_path):
        directory = os.path.dirname(file_path)
    else:
        directory = file_path
    if directory and not os.path.exists(directory):
        os.makedirs(directory, exist_ok=True)


def process_paths(input_path, output_path):
    # Extract input directory and filename
    input_dir = os.path.dirname(input_path)
    input_filename = os.path.basename(input_path)

    # Check if output path has an extension (meaning it's a file) or is a directory
    output_has_extension = "." in os.path.basename(output_path)

    if output_has_extension:
        output_dir = os.path.dirname(output_path)
        output_filename = os.path.basename(output_path)
    else:
        output_dir = output_path  # Assume it's a directory
        output_filename = os.path.splitext(input_filename)[0] + ".ztm"

    # Construct full output path
    full_output_path = os.path.join(output_dir, output_filename)

    return output_dir, output_filename, full_output_path


def _compress_same_seq(data):
    count = 0
    i = 0
    while (i < len(data)) and (data[0] == data[i] and count < 128):
        count += 1
        i += 1

    return count


def _compress_diff_seq(data):
    count = 0
    i = 1
    while (i < len(data)) and (data[i - 1] != data[i] and count < 128):
        count += 1
        i += 1

    return count


def compress(tile: list):
    ret = []
    i = 0
    while i < len(tile):  # TILE_SIZE # 16 * 16
        same_count = _compress_same_seq(tile[i:])
        diff_count = _compress_diff_seq(tile[i:])

        if diff_count > 0:
            ret.append(diff_count - 1)
            for j in range(i, i + diff_count):
                ret.append(tile[j])
            i += diff_count
        else:
            ret.append((same_count - 1) + 0x80)
            ret.append(tile[i])
            i += same_count

    return ret


def get_layers(layer):
    if layer:
        layers = root.findall("layer[@name='" + str(layer) + "']")
    else:
        layers = root.findall("layer")

    results = []
    for layer in layers:
        data = layer.find("data")
        if not data.attrib["encoding"] == "csv":
            print(
                "Invalid Data Encoding, expected CSV and got ", data.attrib["encoding"]
            )
            return None

        # tiled indexes are 1 based, so we subtract 1 to make it 0 based
        tiles = [
            255 if int(i) < 1 else int(i) - 1
            for i in data.text.replace("\n", "").split(",")
        ]
        results.append(tiles)

    return results


def get_screens(args, layer):
    screens = []
    if args.size:
        mw = int(meta["width"])  # map width
        mh = int(meta["height"])  # map height
        sw = int(args.size["width"])  # screen width
        sh = int(args.size["height"])  # screen height
        sx = mw // sw  # horizontal screens
        sy = mh // sh  # vertical screens

        for i in range(sy):  # vertical screens
            for j in range(sx):  # horizontal screens
                world_offset = ((mw * i) * sh) + (sw * j)
                screen = []
                for y in range(sh):  # screen height
                    line = []
                    for x in range(sw):  # screen width
                        offset = world_offset + (mw * y) + x
                        tile = layer[offset]
                        if args.debug:
                            print(
                                "offset",
                                {
                                    "map_y": i,
                                    "map_x": j,
                                    "scr_y": y,
                                    "scr_x": x,
                                    "offset": offset,
                                    "tile": tile,
                                },
                            )
                        line.append(tile)
                    screen = screen + line
                if args.debug:
                    print("screen", len(screen), sh, sw)
                screens.append(bytes(screen))
    else:
        screens.append(bytes(layer))

    return screens


def convert(args):
    keys = {}
    gates = {}
    fuel = {}

    layers = get_layers("objects")
    for layer in layers:
        screens = get_screens(args, layer)
        for y in range(0, WORLD_WIDTH):
            for x in range(0, WORLD_HEIGHT):
                offset = (y * WORLD_WIDTH) + x
                screen = screens[offset]
                print("screen", x, y)
                #  print the header
                print("       ", end="")
                for col in range(40):
                    print(f"{col:02X} ", end="")
                print("\n" + "-" * (40 * 3 + 6))

                # get the hex string for each value
                hex = [f"{byte:02X}" for byte in screen]
                found = False
                for i in range(0, len(screen), 40):
                    # print the header
                    print(f"{i:04x} |", end=" ")

                    for j in range(0, 40):
                        value = screen[i + j]
                        color = ""
                        if (value >= GATE_INDEX) and (value <= GATE_INDEX + GATE_COUNT):
                            """ found a gate """
                            found = True
                            color = DATA_COLOR
                            hex[i + j] = f"{value - GATE_INDEX:02X}"
                        elif (value >= KEY_INDEX) and (value <= KEY_INDEX + GATE_COUNT):
                            """ found a key """
                            found = True
                            color = ROW_INDEX_COLOR
                            hex[i + j] = f"{value - KEY_INDEX:02X}"
                            # convert to 0 indexed, store room id and screen x,y
                            keys[value - KEY_INDEX] = (offset, j, i // 40, value)
                        elif (value == FUEL_INDEX):
                            found = True
                            color = HEADER_COLOR
                            hex[i + j] = f"{value - FUEL_INDEX:02X}"
                            fuel[offset] = (offset, j, i // 40, value)
                        else:
                            if found:
                                found = False
                                color = RESET
                        print(f"{color}{hex[i+j]} ", end="")
                    print("")

    return (keys, gates, fuel)


def main():
    global root, meta
    args = parser.parse_args()
    tree = ET.parse(args.input)
    root = tree.getroot()
    meta = root.attrib

    if args.verbose:
        print("args", args)
        print("root", root)
        print("meta", meta)

    keys,gates,fuel = convert(args)

    print("keys", keys)
    print("gates", gates)

    with open("src/objects.c", "w") as file:
        file.write("#include \"objects.h\"\n\n\n")

        # object_t keys[] = {
        #     { .room = 0, .x = 1, .y = 2, },
        #     { .room = 2, .x = 1, .y = 2, }
        # };
        # const uint8_t keys_len = 9;
        file.write("object_t keys[] = {\n")
        for i in sorted(keys):
            key = keys[i]
            file.write("\t{ ")
            file.write(f".room = {key[0]}, ")
            file.write(f".x = {key[1]}, ")
            file.write(f".y = {key[2]}, ")
            file.write(f".flags = 0x01, ")
            file.write("}, ")
            file.write(f"// value = {key[3]}")
            file.write("\n")
        file.write("};\n")
        file.write(f"const uint8_t keys_len = {len(keys)};\n\n")

        # object_t keys[TOTAL_GATES] = {
        #     { .room = 0, .x = 1, .y = 2, },
        #     { .room = 2, .x = 1, .y = 2, }
        # };
        # const uint8_t keys_len = 9;
        file.write("object_t fuel[] = {\n")
        for i in sorted(fuel):
            can = fuel[i]
            file.write("\t{ ")
            file.write(f".room = {can[0]}, ")
            file.write(f".x = {can[1]}, ")
            file.write(f".y = {can[2]}, ")
            file.write(f".flags = 0x01, ")
            file.write("}, ")
            file.write(f"// value = {can[3]}")
            file.write("\n")
        file.write("};\n")
        file.write(f"const uint8_t fuel_len = {len(fuel)};\n\n")

if __name__ == "__main__":
    main()
