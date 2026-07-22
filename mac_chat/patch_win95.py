import re

with open("main.mm", "r") as f:
    content = f.read()

win95_classes = """
@interface Win95Button : NSButton
@end
@implementation Win95Button
- (void)drawRect:(NSRect)dirtyRect {
    if (self.buttonType == NSButtonTypeSwitch || self.buttonType == NSButtonTypeRadio) {
        NSRect bounds = self.bounds;
        NSRect boxRect = NSMakeRect(0, (bounds.size.height - 13)/2.0, 13, 13);
        [[NSColor whiteColor] setFill];
        NSRectFill(boxRect);
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(boxRect.origin.x, boxRect.origin.y + boxRect.size.height - 1, boxRect.size.width, 1));
        NSRectFill(NSMakeRect(boxRect.origin.x, boxRect.origin.y, 1, boxRect.size.height));
        [[NSColor whiteColor] set];
        NSRectFill(NSMakeRect(boxRect.origin.x, boxRect.origin.y, boxRect.size.width, 1));
        NSRectFill(NSMakeRect(boxRect.origin.x + boxRect.size.width - 1, boxRect.origin.y, 1, boxRect.size.height));
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(boxRect.origin.x+1, boxRect.origin.y + boxRect.size.height - 2, boxRect.size.width-2, 1));
        NSRectFill(NSMakeRect(boxRect.origin.x+1, boxRect.origin.y+1, 1, boxRect.size.height-2));
        [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] set];
        NSRectFill(NSMakeRect(boxRect.origin.x+1, boxRect.origin.y+1, boxRect.size.width-2, 1));
        NSRectFill(NSMakeRect(boxRect.origin.x + boxRect.size.width - 2, boxRect.origin.y+1, 1, boxRect.size.height-2));
        if (self.state == NSControlStateValueOn) {
            [[NSColor blackColor] set];
            NSBezierPath *path = [NSBezierPath bezierPath];
            [path moveToPoint:NSMakePoint(boxRect.origin.x + 3, boxRect.origin.y + 6)];
            [path lineToPoint:NSMakePoint(boxRect.origin.x + 5, boxRect.origin.y + 4)];
            [path lineToPoint:NSMakePoint(boxRect.origin.x + 10, boxRect.origin.y + 9)];
            [path setLineWidth:1.5];
            [path stroke];
        }
        NSDictionary *attrs = @{NSFontAttributeName: self.font ? self.font : [NSFont systemFontOfSize:12], NSForegroundColorAttributeName: [NSColor blackColor]};
        NSSize size = [self.title sizeWithAttributes:attrs];
        [self.title drawAtPoint:NSMakePoint(18, (bounds.size.height - size.height)/2.0) withAttributes:attrs];
        return;
    }
    NSRect bounds = self.bounds;
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
    NSRectFill(bounds);
    BOOL pressed = self.isHighlighted || self.state == NSControlStateValueOn;
    if (pressed) {
        [[NSColor blackColor] set];
        NSFrameRectWithWidth(bounds, 1.0);
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSFrameRectWithWidth(NSInsetRect(bounds, 1, 1), 1.0);
    } else {
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1));
        NSRectFill(NSMakeRect(bounds.size.width-1, 0, 1, bounds.size.height));
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(1, 1, bounds.size.width-2, 1));
        NSRectFill(NSMakeRect(bounds.size.width-2, 1, 1, bounds.size.height-2));
        [[NSColor whiteColor] set]; 
        NSRectFill(NSMakeRect(0, bounds.size.height-1, bounds.size.width, 1));
        NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height));
        [[NSColor colorWithCalibratedRed:0.85 green:0.85 blue:0.85 alpha:1.0] set]; 
        NSRectFill(NSMakeRect(1, bounds.size.height-2, bounds.size.width-2, 1));
        NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height-2));
    }
    NSMutableParagraphStyle *style = [[NSMutableParagraphStyle alloc] init];
    style.alignment = NSTextAlignmentCenter;
    NSDictionary *attrs = @{NSFontAttributeName: self.font ? self.font : [NSFont systemFontOfSize:12],
                            NSForegroundColorAttributeName: [NSColor blackColor],
                            NSParagraphStyleAttributeName: style};
    NSSize size = [self.title sizeWithAttributes:attrs];
    NSRect titleRect = NSMakeRect(0, (bounds.size.height - size.height) / 2.0, bounds.size.width, size.height);
    if (pressed) { titleRect.origin.x += 1; titleRect.origin.y -= 1; }
    if (self.image) {
        NSSize imgSize = self.image.size;
        NSRect imgRect = NSMakeRect((bounds.size.width - imgSize.width)/2.0, (bounds.size.height - imgSize.height)/2.0, imgSize.width, imgSize.height);
        if (pressed) { imgRect.origin.x += 1; imgRect.origin.y -= 1; }
        [self.image drawInRect:imgRect];
    } else {
        [self.title drawInRect:titleRect withAttributes:attrs];
    }
}
@end

@interface Win95TextField : NSTextField
@end
@implementation Win95TextField
- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) { self.bordered = NO; self.drawsBackground = YES; self.backgroundColor = [NSColor whiteColor]; }
    return self;
}
- (void)drawRect:(NSRect)dirtyRect {
    NSRect bounds = self.bounds;
    [super drawRect:dirtyRect];
    [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
    NSRectFill(NSMakeRect(0, bounds.size.height-1, bounds.size.width, 1));
    NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height));
    [[NSColor whiteColor] set];
    NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1));
    NSRectFill(NSMakeRect(bounds.size.width-1, 0, 1, bounds.size.height));
    [[NSColor blackColor] set];
    NSRectFill(NSMakeRect(1, bounds.size.height-2, bounds.size.width-2, 1));
    NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height-2));
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] set];
    NSRectFill(NSMakeRect(1, 1, bounds.size.width-2, 1));
    NSRectFill(NSMakeRect(bounds.size.width-2, 1, 1, bounds.size.height-2));
}
@end

@interface Win95ScrollView : NSScrollView
@end
@implementation Win95ScrollView
- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) { self.borderType = NSNoBorder; self.drawsBackground = YES; self.backgroundColor = [NSColor whiteColor]; }
    return self;
}
- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    NSRect bounds = self.bounds;
    [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
    NSRectFill(NSMakeRect(0, bounds.size.height-1, bounds.size.width, 1));
    NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height));
    [[NSColor whiteColor] set];
    NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1));
    NSRectFill(NSMakeRect(bounds.size.width-1, 0, 1, bounds.size.height));
    [[NSColor blackColor] set];
    NSRectFill(NSMakeRect(1, bounds.size.height-2, bounds.size.width-2, 1));
    NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height-2));
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] set];
    NSRectFill(NSMakeRect(1, 1, bounds.size.width-2, 1));
    NSRectFill(NSMakeRect(bounds.size.width-2, 1, 1, bounds.size.height-2));
}
@end
"""

# Insert classes after imports
insert_pos = content.find("@class VideoWindowController;")
content = content[:insert_pos] + win95_classes + "\n" + content[insert_pos:]

content = content.replace("[[NSButton alloc]", "[[Win95Button alloc]")
content = content.replace("[[NSTextField alloc]", "[[Win95TextField alloc]")
content = content.replace("[[NSScrollView alloc]", "[[Win95ScrollView alloc]")

# Window backgrounds
content = content.replace("win.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];", 
                          "win.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];\n    win.backgroundColor = [NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0];")

content = content.replace("self.window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];", 
                          "self.window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];\n    self.window.backgroundColor = [NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0];")

with open("main.mm", "w") as f:
    f.write(content)
