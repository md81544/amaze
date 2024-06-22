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
# by their ID value set in inkscape ('exit', 'start', 'fuel')
circles = {}
for circle in doc.getElementsByTagName('circle'):
    id = circle.getAttribute('id')
    x = int(float(circle.getAttribute('cx')))
    y = int(float(circle.getAttribute('cy')))
    circles[id] = (x,y)
print(circles)
exit()
doc.unlink()

items = []
for path in path_strings:
    items = items + path.split()

first_x = 0
first_y = 0
last_x = 0
last_y = 0
current_command = ''

for item in items:
    match item:
        case 'M': # move
            current_command = item;
        case 'V': # vertical line
            current_command = item;
        case 'H': # horizontal line
            current_command = item;
        case 'L': # line
            current_command = item;
        case 'Z': # home to first point
            print("HOME ")
            continue
        case _:
            print(f"{current_command} {item}")