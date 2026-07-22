import re

with open("main.mm", "r") as f:
    content = f.read()

# 1. Update Layout
old_layout = """    self.voiceButton.frame = NSMakeRect(currentX, toolY, 75, toolHeight);
    currentX += 80;"""

new_layout = """    self.voiceButton.frame = NSMakeRect(currentX, toolY, 65, toolHeight);
    currentX += 70;"""

content = content.replace(old_layout, new_layout)

# 2. Update Win95Button mic/text centering
old_draw = """    if (self.hasMicIcon) {
        CGFloat cx = round(bounds.size.width / 2.0 - 20);
        CGFloat cy = round(bounds.size.height / 2.0);
        if (pressed) { cx += 1; cy += 1; }
        
        // Draw Light Gray Inner Head"""

new_draw = """    if (self.hasMicIcon) {
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
        if (pressed) { titleRect.origin.x += 1; }
        
        // Draw Light Gray Inner Head"""

content = content.replace(old_draw, new_draw)

# We also need to remove the old titleRect shift at the end of hasMicIcon block
old_shift = """        // Base
        NSRectFill(NSMakeRect(cx - 2, cy + 6, 5, 1));
        NSRectFill(NSMakeRect(cx - 3, cy + 7, 7, 1));
        
        titleRect.origin.x += 12;
    }"""

new_shift = """        // Base
        NSRectFill(NSMakeRect(cx - 2, cy + 6, 5, 1));
        NSRectFill(NSMakeRect(cx - 3, cy + 7, 7, 1));
    }"""

content = content.replace(old_shift, new_shift)

with open("main.mm", "w") as f:
    f.write(content)
