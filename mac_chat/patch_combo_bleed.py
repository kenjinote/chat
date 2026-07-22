import re

with open("main.mm", "r") as f:
    content = f.read()

old_draw = """    // 2. Draw the Windows 95 button over the right side
    CGFloat btnWidth = 16.0;
    NSRect btnRect = NSMakeRect(bounds.size.width - btnWidth - 2, 2, btnWidth, bounds.size.height - 4);"""

new_draw = """    // 2. Draw the Windows 95 button over the right side
    CGFloat btnWidth = 16.0;
    NSRect btnRect = NSMakeRect(bounds.size.width - btnWidth - 2, 2, btnWidth, bounds.size.height - 4);
    
    // Erase the native macOS blue arrow that bleeds out to the left of our button
    [[NSColor whiteColor] setFill];
    NSRect eraseRect = NSMakeRect(btnRect.origin.x - 6, 2, 6, bounds.size.height - 4);
    NSRectFill(eraseRect);"""

content = content.replace(old_draw, new_draw)

with open("main.mm", "w") as f:
    f.write(content)
