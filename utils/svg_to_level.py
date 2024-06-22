#!/usr/bin/env python3
import argparse
from xml.dom import minidom

parser = argparse.ArgumentParser("svg_to_level")
parser.add_argument("svgfile", help="Filename to convert", type=str)
args = parser.parse_args()

doc = minidom.parse(args.svgfile)
path_strings = [path.getAttribute('d') for path in doc.getElementsByTagName('path')]
doc.unlink()

items = []
for path in path_strings:
    items = items + path.split()

last_x = 0
last_y = 0

for item in items:
    print(item)
    # TODO process items