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
circles = []
for circle in doc.getElementsByTagName('circle'):
    id = circle.getAttribute('id')
    label = circle.getAttribute('inkscape:label')
    x = int(float(circle.getAttribute('cx')))
    y = int(float(circle.getAttribute('cy')))
    circles.append((label,x,y))
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
            print(f"LINE from {last_x},{last_y} to {first_x},{first_y}")
            continue
        case _:
            if (item or 'x')[0] .isdigit():
                if ',' in item:
                    (xs,ys) = item.split(',')
                    x = int(float(xs))
                    y = int(float(ys))
                else:
                    amt = int(float(item))
                if current_command == 'M':
                    print(f"MOVE to {x},{y}")
                    first_x = x
                    first_y = y
                    last_x = x
                    last_y = y
                elif current_command == 'L':
                    print(f"LINE from {last_x},{last_y} to {x},{y}")
                    last_x = x
                    last_y = y
                elif current_command == 'H':
                    print(f"HORIZONTAL from {last_x},{last_y} to {amt},{last_y}")
                    last_x = amt
                    last_y = y
                elif current_command == 'V':
                    print(f"VERTICAL from {last_x},{last_y} to {last_x},{amt}")
                    last_x = x
                    last_y = amt
