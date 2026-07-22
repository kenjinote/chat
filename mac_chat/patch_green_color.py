import re

with open("main.mm", "r") as f:
    content = f.read()

old_color = """[[NSColor colorWithCalibratedRed:0.0 green:0.7 blue:0.0 alpha:1.0] setFill];"""
new_color = """[[NSColor colorWithCalibratedRed:0.0 green:1.0 blue:0.0 alpha:1.0] setFill];"""

content = content.replace(old_color, new_color)

with open("main.mm", "w") as f:
    f.write(content)
