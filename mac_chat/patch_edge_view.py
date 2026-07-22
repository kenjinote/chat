import re

with open("main.mm", "r") as f:
    content = f.read()

# 1. Add Win95EdgeView
win95_edge_view = """@interface Win95EdgeView : NSView
@end
@implementation Win95EdgeView
- (BOOL)isFlipped { return YES; }
- (void)drawRect:(NSRect)dirtyRect {
    NSRect bounds = self.bounds;
    [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] setFill];
    NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1));
    NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height));
    [[NSColor blackColor] setFill];
    NSRectFill(NSMakeRect(1, 1, bounds.size.width - 2, 1));
    NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height - 2));
    [[NSColor whiteColor] setFill];
    NSRectFill(NSMakeRect(0, bounds.size.height - 1, bounds.size.width, 1));
    NSRectFill(NSMakeRect(bounds.size.width - 1, 0, 1, bounds.size.height));
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
    NSRectFill(NSMakeRect(1, bounds.size.height - 2, bounds.size.width - 2, 1));
    NSRectFill(NSMakeRect(bounds.size.width - 2, 1, 1, bounds.size.height - 2));
}
@end
"""

# Insert it before Win95ScrollView
content = content.replace("@interface Win95ScrollView : NSScrollView", win95_edge_view + "\n@interface Win95ScrollView : NSScrollView")

with open("main.mm", "w") as f:
    f.write(content)
