import re

with open("main.mm", "r") as f:
    content = f.read()

win95_colorwell = """@interface Win95ColorWell : NSColorWell
@end

@implementation Win95ColorWell
- (BOOL)isFlipped { return YES; }

- (void)drawRect:(NSRect)dirtyRect {
    NSRect bounds = self.bounds;
    BOOL pressed = [self isActive]; // active means color panel is open
    
    // 1. Draw Button Background
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
    NSRectFill(bounds);
    
    // 2. Draw 3D Button Bevel
    if (pressed) {
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1));
        NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height));
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(1, 1, bounds.size.width-2, 1));
        NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height-2));
    } else {
        [[NSColor whiteColor] set];
        NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1));
        NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height));
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(0, bounds.size.height - 1, bounds.size.width, 1));
        NSRectFill(NSMakeRect(bounds.size.width - 1, 0, 1, bounds.size.height));
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(1, bounds.size.height - 2, bounds.size.width-2, 1));
        NSRectFill(NSMakeRect(bounds.size.width - 2, 1, 1, bounds.size.height-2));
    }
    
    // 3. Draw Square Color Preview
    CGFloat squareSize = 12.0;
    NSRect colorRect = NSMakeRect(round((bounds.size.width - squareSize) / 2.0), round((bounds.size.height - squareSize) / 2.0), squareSize, squareSize);
    
    if (pressed) {
        colorRect.origin.x += 1;
        colorRect.origin.y += 1;
    }
    
    // Black border around the color square
    [[NSColor blackColor] setFill];
    NSRect borderRect = NSMakeRect(colorRect.origin.x - 1, colorRect.origin.y - 1, colorRect.size.width + 2, colorRect.size.height + 2);
    NSRectFill(borderRect);
    
    // The actual selected color
    [self.color setFill];
    NSRectFill(colorRect);
}
@end
"""

insert_idx = content.find("@interface Win95ScrollView")
content = content[:insert_idx] + win95_colorwell + "\n" + content[insert_idx:]

content = content.replace("self.colorWell = [[NSColorWell alloc]", "self.colorWell = [[Win95ColorWell alloc]")
content = content.replace("NSColorWell *colorWell = [[NSColorWell alloc]", "NSColorWell *colorWell = [[Win95ColorWell alloc]")

with open("main.mm", "w") as f:
    f.write(content)
