#!/usr/bin/env python3
import argparse
from xml.dom import minidom

parser = argparse.ArgumentParser("svg_to_level")
parser.add_argument("svgfile", help="Filename to convert", type=str)
args = parser.parse_args()

doc = minidom.parse(args.svgfile)
# paths are used for the maze walls
path_strings = [path.getAttribute('d') for path in doc.getElementsByTagName('path')]
# circles are used to denote everything else, e.g. fuel, start, exit. They are distinguished
# by their inkscape:label value set in inkscape ('exit', 'start', 'fuel')
items = []
for item in doc.getElementsByTagName('circle'):
    id = item.getAttribute('id')
    label = item.getAttribute('inkscape:label')
    x = int(float(item.getAttribute('cx')))
    y = int(float(item.getAttribute('cy')))
    items.append((label,x,y))
doc.unlink()

line_commands = []
for path in path_strings:
    line_commands = line_commands + path.split()

first_x = 0
first_y = 0
last_x = 0
last_y = 0
current_command = ''

# First look for the ship's start coordinates to add to the first line:
for item in items:
    if item[0] == 'start':
        print(f"!~0~0~{item[1]}~{item[2]}")

for lc in line_commands:
    match lc:
        case 'M': # move
            print("N~OBSTRUCTION");
            current_command = lc;
        case 'V': # vertical line
            current_command = lc;
        case 'H': # horizontal line
            current_command = lc;
        case 'L': # line
            current_command = lc;
        case 'Z': # home to first point
            print(f"L~{last_x}~{last_y}~{first_x}~{first_y}~255~0~0~0")
            continue
        case _:
            if (lc or 'x')[0] .isdigit():
                if ',' in lc:
                    (xs,ys) = lc.split(',')
                    x = int(float(xs))
                    y = int(float(ys))
                else:
                    amt = int(float(lc))
                if current_command == 'M':
                    first_x = x
                    first_y = y
                    last_x = x
                    last_y = y
                elif current_command == 'L':
                    print(f"L~{last_x}~{last_y}~{x}~{y}~255~0~0~0")
                    last_x = x
                    last_y = y
                elif current_command == 'H':
                    print(f"L~{last_x}~{last_y}~{amt}~{y}~255~0~0~0")
                    last_x = amt
                    last_y = y
                elif current_command == 'V':
                    print(f"L~{last_x}~{last_y}~{x}~{amt}~255~0~0~0")
                    last_x = x
                    last_y = amt
                if current_command == 'M':
                    current_command = 'L'

# Other items
for item in items:
    if item[0] != 'start':
        print()
