import re

with open("main.mm", "r") as f:
    content = f.read()

win95_combo = """@interface Win95ComboBox : NSComboBox
@end
@implementation Win95ComboBox
- (BOOL)isFlipped { return YES; }

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        self.bordered = NO;
        self.backgroundColor = [NSColor whiteColor];
        self.drawsBackground = YES;
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    NSRect bounds = self.bounds;
    
    // 1. Draw Sunken Border around the text field part
    [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
    NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1)); // Top
    NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height)); // Left
    
    [[NSColor blackColor] set];
    NSRectFill(NSMakeRect(1, 1, bounds.size.width-2, 1));
    NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height-2));
    
    [[NSColor whiteColor] set];
    NSRectFill(NSMakeRect(0, bounds.size.height - 1, bounds.size.width, 1));
    NSRectFill(NSMakeRect(bounds.size.width - 1, 0, 1, bounds.size.height));
    
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] set];
    NSRectFill(NSMakeRect(1, bounds.size.height - 2, bounds.size.width - 2, 1));
    NSRectFill(NSMakeRect(bounds.size.width - 2, 1, 1, bounds.size.height - 2));
    
    // 2. Draw the Windows 95 button over the right side
    CGFloat btnWidth = 16.0;
    NSRect btnRect = NSMakeRect(bounds.size.width - btnWidth - 2, 2, btnWidth, bounds.size.height - 4);
    
    // We cover whatever was drawn under this rectangle
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
    NSRectFill(btnRect);
    
    BOOL pressed = [self.cell isHighlighted];
    
    // Raised 3D Button Border
    if (pressed) {
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(btnRect.origin.x, btnRect.origin.y, btnRect.size.width, 1));
        NSRectFill(NSMakeRect(btnRect.origin.x, btnRect.origin.y, 1, btnRect.size.height));
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(btnRect.origin.x+1, btnRect.origin.y+1, btnRect.size.width-2, 1));
        NSRectFill(NSMakeRect(btnRect.origin.x+1, btnRect.origin.y+1, 1, btnRect.size.height-2));
    } else {
        [[NSColor whiteColor] set];
        NSRectFill(NSMakeRect(btnRect.origin.x, btnRect.origin.y, btnRect.size.width, 1));
        NSRectFill(NSMakeRect(btnRect.origin.x, btnRect.origin.y, 1, btnRect.size.height));
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(btnRect.origin.x, btnRect.origin.y + btnRect.size.height - 1, btnRect.size.width, 1));
        NSRectFill(NSMakeRect(btnRect.origin.x + btnRect.size.width - 1, btnRect.origin.y, 1, btnRect.size.height));
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(btnRect.origin.x+1, btnRect.origin.y + btnRect.size.height - 2, btnRect.size.width-2, 1));
        NSRectFill(NSMakeRect(btnRect.origin.x + btnRect.size.width - 2, btnRect.origin.y+1, 1, btnRect.size.height-2));
    }
    
    // Pixel-perfect Down Arrow (centered in the button)
    CGFloat cx = round(NSMidX(btnRect));
    CGFloat cy = round(NSMidY(btnRect));
    if (pressed) { cx += 1; cy += 1; }
    
    [[NSColor blackColor] setFill];
    NSRectFill(NSMakeRect(cx - 3, cy - 1, 7, 1));
    NSRectFill(NSMakeRect(cx - 2, cy, 5, 1));
    NSRectFill(NSMakeRect(cx - 1, cy + 1, 3, 1));
    NSRectFill(NSMakeRect(cx, cy + 2, 1, 1));
}
@end
"""

insert_idx = content.find("@interface Win95ScrollView")
content = content[:insert_idx] + win95_combo + "\n" + content[insert_idx:]

content = content.replace("self.sizeCombo = [[NSComboBox alloc] initWithFrame:NSZeroRect];", "self.sizeCombo = [[Win95ComboBox alloc] initWithFrame:NSZeroRect];")

with open("main.mm", "w") as f:
    f.write(content)
