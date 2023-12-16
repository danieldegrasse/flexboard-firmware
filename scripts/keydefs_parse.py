import argparse
import csv

def pretty_print(elements, limit=80):
    """
    Helper to 'pretty print' a block of definitions.
    prints up to 'limit' columns of characters per line
    """
    output = ""
    line_len = 0
    for e in elements:
        line_len += len(e)
        if line_len >= limit:
            # New line
            output += "\n"
            line_len = 0
            output += e
            output += " "
        else:
            output += e
            output += " "
    output = output[:-1] + ";"
    print(output)

parser = argparse.ArgumentParser(prog='keydefs_parse',
                                 description="""Parse key definition CSV. Outputs useful
                                    devicetree data needed for ZMK firmware configuration
                                    """)
parser.add_argument('csv', help='Key definition CSV to parse')
args = parser.parse_args()

csvfile = open(args.csv, "r")
keyreader = csv.reader(csvfile)
# Read CSV into dict
keys = {}
for row in keyreader:
    if row[0] == "Key name":
        # Header row
        continue
    keys[row[0]] = {"row": int(row[1]), "col": int(row[2]),
                    "source": int(row[3]), "sink": int(row[4])}
# Find max and min LED line indices, so that we can make valid pixel coordinates
led_sources = []
led_sinks = []
led_coords = {}
for coord in keys.values():
    led_sources.append(coord["source"])
    led_sinks.append(coord["sink"])
    # Put a placeholder for the pixel index. We will calculate it later.
    led_coords[((coord["source"], coord["sink"]))] = -1
source_min = min(led_sources) - 1
source_range = max(led_sources) - min(led_sources)
sink_min = min(led_sinks) - 1
sink_range = max(led_sinks) - min(led_sinks)

chain_reserved_ranges = {}
print("Pixel Definitions")
# IS31FL3733B controller has 12 sinks, 16 sources
# We calculate pixel definitions based on which LEDs are actually populated,
# but we iterate over the entire possible LED space so the chain-reserved-ranges
# property will be accurate.
pixels = []
pixel_idx = 0
for sink in range(12):
    for source in range(16):
        # Calculate pixel offset
        if source % 2 == 0:
            sink_coord =  (2 * sink_range) - (2 * (sink - sink_min))
        else:
            sink_coord =  ((2 * sink_range) + 1) - (2 * (sink - sink_min))
        x_coord = int((sink_coord / ((2 * sink_range) + 1)) * 255)
        y_coord = int((((source - source_min) // 2) / (source_range // 2)) * 255)
        if ((source + 1), (sink + 1)) in led_coords:
            pixels.append(f"<&pixel {x_coord} {y_coord}>, "
                          f"/* LED_SINK_{12 - sink}, LED_SOURCE_{source + 1} */")
            # Record pixel index in pixels array
            led_coords[(source + 1), (sink + 1)] = pixel_idx
            pixel_idx += 1
        else:
            pixels.append(f"/* LED_SINK_{12 - sink}, "
                          f"LED_SOURCE_{source + 1} is not populated */")
            # Add to the chain-reserved-ranges property
            offset = ((sink * 16) + source) + 1
            offset_found = False
            for (key, value) in chain_reserved_ranges.items():
                if (key + value) == offset:
                    # Extend current reserved range
                    chain_reserved_ranges[key] += 1
                    offset_found = True
                    break
            if not offset_found:
                # Add a new offset
                chain_reserved_ranges[offset] = 1
pretty_print(pixels, limit=30)
print("Chain reserved ranges")
chain_ranges = []
for (offset,length) in chain_reserved_ranges.items():
    chain_ranges.append(f"<{offset} {length}>,")
pretty_print(chain_ranges, limit=40)

key_pixels = []
# Print key pixel definitions
for (key, coord) in keys.items():
    offset = led_coords[(coord["source"], coord["sink"])]
    key_pixels.append(str(offset))
print("Key pixels")
pretty_print(key_pixels, limit=30)

# Print key zones
print("Key zones")
zones = []
for i in range(len(keys)):
    zones.append(str(i))
pretty_print(zones, limit=50)

# Print key definitions
print("Key definitions")
for (key, coord) in keys.items():
    print(f"#define {key} RC({coord['row'] - 1},{coord['col'] - 1})")

# Print keymap
print("Key map")
pretty_print(keys.keys())
