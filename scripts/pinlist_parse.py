import re
import argparse

def make_part_dict(pinlist_fname):
    """
    Makes dictionary of parts. Keys are part names, values are dictionary
    mapping pin names to net names
    @param pinlist_fname: pinlist filename to parse
    """
    part_re = re.compile(r'(\S+)\s*')
    pinlist = open(pinlist_fname,"r")
    parts = {}

    # Consume first 8 lines, they are just headers
    for i in range(8):
        line = pinlist.readline()
    while line != "":
        # Parse line with regex
        matches = part_re.findall(line)
        # If match count is 5, we have a part row
        if len(matches) == 5:
            part = matches[0]
            net_dict = {matches[2]: matches[4]}
            # Read next lines for more pin -> net mappings
            line = pinlist.readline()
            matches = part_re.findall(line)
            while len(matches) == 4 and line != "":
                net_dict[matches[1]] = matches[3]
                line = pinlist.readline()
                matches = part_re.findall(line)
            parts[part] = net_dict
        # Next line
        line = pinlist.readline()

    # Done with pinlist file
    pinlist.close()
    return parts

def write_keymap(parts, csv_f):
    """
    Writes a keymap csv to the provided file, using the parts
    dictionary as a source for the key routing
    @param parts: dictionary mapping parts to map of pin names to net names
    @param csv_f: filename to output csv to
    """
    key_re = re.compile(r'KEY_(\d+)')
    key_col_re = re.compile(r'COL_(\d+)')
    key_row_re = re.compile(r'ROW_(\d+)')
    led_source_re = re.compile(r'LED_SOURCE_(\d+)')
    led_soruce_re = re.compile(r'LED_SORUCE_(\d+)')
    led_sink_re = re.compile(r'LED_SINK_(\d+)')

    # Now, create a mapping of keys to led source/sink, and key row/columns
    keymap = {}
    for (part,pins) in parts.items():
        keymatch = key_re.match(part)
        if keymatch:
            # key row, key col, led source, led sink
            coords = [-1, -1, -1, -1]
            for (pin,net) in pins.items():
                if pin == "SW1":
                    coords[1] = int(key_col_re.match(net).group(1))
                elif pin == "LED+":
                    match = led_source_re.match(net)
                    if match is None:
                        # Someone (me) misspelled a net name
                        match = led_soruce_re.match(net)
                    coords[2] = int(match.group(1))
                elif pin == "LED-":
                    coords[3] = 13 - int(led_sink_re.match(net).group(1))
                elif pin == "SW2":
                    # Switch 2 is connected to a diode. We have to follow the net
                    # trail through the diode to find the key row
                    for (part, pins) in parts.items():
                        match = None
                        if net in pins.values():
                            for diode_net in pins.values():
                                match = key_row_re.match(diode_net)
                                if match:
                                    coords[0] = int(match.group(1))
                                    break
                        if match:
                            # Search is done
                            break
            keymap[keymatch.group(0)] = coords
    csv_file = open(csv_f, "w")
    csv_file.write("Key name,Key Row,Key Column,LED Source,LED Sink\n")
    for key,coords in keymap.items():
        line = f"{key},"
        for coord in coords:
            line += (str(coord) + ",")
        # Strip last comma
        line = line[:-1]
        csv_file.write(line + "\n")
    csv_file.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog='pinlist_parser',
                                     description='Parse Eagle pinlist to create key csv')
    parser.add_argument('pinlist', help='Eagle pinlist filename to parse')
    parser.add_argument('csv', help='Filename to write output key csv to')

    args = parser.parse_args()
    part_dict = make_part_dict(args.pinlist)
    write_keymap(part_dict, args.csv)
    print(f"Wrote keymap to {args.csv}")
