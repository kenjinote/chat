import re

with open("main.mm", "r") as f:
    content = f.read()

old_draw = """        CGFloat cx = startX + (micWidth / 2.0);
        CGFloat cy = round(bounds.size.height / 2.0);"""

new_draw = """        CGFloat cx = startX + (micWidth / 2.0) - 3.0; // Moved 3px left per user request
        CGFloat cy = round(bounds.size.height / 2.0);"""

content = content.replace(old_draw, new_draw)

with open("main.mm", "w") as f:
    f.write(content)
