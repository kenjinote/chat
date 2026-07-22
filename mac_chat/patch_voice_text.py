import re

with open("main.mm", "r") as f:
    content = f.read()

old_draw = """    if (self.hasMicIcon) {
        // Tightly group mic and text, and exactly center them inside the button
        CGFloat gap = 4.0;
        CGFloat micWidth = 10.0;
        CGFloat totalBlockWidth = micWidth + gap + size.width;
        CGFloat startX = round((bounds.size.width - totalBlockWidth) / 2.0);
        
        CGFloat cx = startX + (micWidth / 2.0);
        CGFloat cy = round(bounds.size.height / 2.0);
        if (pressed) { cx += 1; cy += 1; }
        
        // Ensure the title is exactly positioned after the mic
        titleRect.origin.x = startX + micWidth + gap;
        if (pressed) { titleRect.origin.x += 1; }"""

new_draw = """    if (self.hasMicIcon) {
        // Tightly group mic and text, and exactly center them inside the button
        CGFloat gap = 2.0; // tighter gap for Windows 95 style
        CGFloat micWidth = 10.0;
        CGFloat totalBlockWidth = micWidth + gap + size.width;
        CGFloat startX = round((bounds.size.width - totalBlockWidth) / 2.0);
        
        CGFloat cx = startX + (micWidth / 2.0);
        CGFloat cy = round(bounds.size.height / 2.0);
        if (pressed) { cx += 1; cy += 1; }
        
        // Ensure the title is exactly positioned after the mic
        titleRect.origin.x = startX + micWidth + gap;
        titleRect.size.width = size.width; // fix text shifting out of bounds due to center alignment!
        if (pressed) { titleRect.origin.x += 1; }"""

content = content.replace(old_draw, new_draw)

with open("main.mm", "w") as f:
    f.write(content)
