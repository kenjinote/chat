import re

with open("main.mm", "r") as f:
    content = f.read()

win95_classes_new = """@interface Win95Button : NSButton
@property (assign) BOOL isCheckbox;
@end
@implementation Win95Button
- (BOOL)isFlipped { return YES; }
- (void)setButtonType:(NSButtonType)type {
    [super setButtonType:type];
    if (type == NSButtonTypeSwitch || type == NSButtonTypeRadio) {
        self.isCheckbox = YES;
    }
}
- (void)drawRect:(NSRect)dirtyRect {
    if (self.isCheckbox) {
        NSRect bounds = self.bounds;
        NSRect boxRect = NSMakeRect(0, (bounds.size.height - 13)/2.0, 13, 13);
        
        // Background
        [[NSColor whiteColor] setFill];
        NSRectFill(boxRect);
        
        // Sunken box
        // Outer Top & Left (Dark Gray)
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(boxRect.origin.x, boxRect.origin.y, boxRect.size.width, 1)); // Top
        NSRectFill(NSMakeRect(boxRect.origin.x, boxRect.origin.y, 1, boxRect.size.height)); // Left
        
        // Inner Top & Left (Black)
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(boxRect.origin.x+1, boxRect.origin.y+1, boxRect.size.width-2, 1));
        NSRectFill(NSMakeRect(boxRect.origin.x+1, boxRect.origin.y+1, 1, boxRect.size.height-2));
        
        // Outer Bottom & Right (White)
        [[NSColor whiteColor] set];
        NSRectFill(NSMakeRect(boxRect.origin.x, boxRect.origin.y + boxRect.size.height - 1, boxRect.size.width, 1));
        NSRectFill(NSMakeRect(boxRect.origin.x + boxRect.size.width - 1, boxRect.origin.y, 1, boxRect.size.height));
        
        // Inner Bottom & Right (Light Gray)
        [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] set];
        NSRectFill(NSMakeRect(boxRect.origin.x+1, boxRect.origin.y + boxRect.size.height - 2, boxRect.size.width-2, 1));
        NSRectFill(NSMakeRect(boxRect.origin.x + boxRect.size.width - 2, boxRect.origin.y+1, 1, boxRect.size.height-2));
        
        if (self.state == NSControlStateValueOn) {
            [[NSColor blackColor] set];
            NSBezierPath *path = [NSBezierPath bezierPath];
            [path moveToPoint:NSMakePoint(boxRect.origin.x + 3, boxRect.origin.y + 6)];
            [path lineToPoint:NSMakePoint(boxRect.origin.x + 5, boxRect.origin.y + 8)];
            [path lineToPoint:NSMakePoint(boxRect.origin.x + 10, boxRect.origin.y + 3)];
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
        // Sunken (Pressed)
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1)); // Top
        NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height)); // Left
        
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(1, 1, bounds.size.width-2, 1)); // Inner Top
        NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height-2)); // Inner Left
        
        [[NSColor whiteColor] set];
        NSRectFill(NSMakeRect(0, bounds.size.height-1, bounds.size.width, 1)); // Bottom
        NSRectFill(NSMakeRect(bounds.size.width-1, 0, 1, bounds.size.height)); // Right
    } else {
        // Raised (Unpressed)
        [[NSColor whiteColor] set];
        NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1)); // Top
        NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height)); // Left
        
        [[NSColor colorWithCalibratedRed:0.85 green:0.85 blue:0.85 alpha:1.0] set];
        NSRectFill(NSMakeRect(1, 1, bounds.size.width-2, 1)); // Inner Top
        NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height-2)); // Inner Left
        
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(0, bounds.size.height-1, bounds.size.width, 1)); // Bottom
        NSRectFill(NSMakeRect(bounds.size.width-1, 0, 1, bounds.size.height)); // Right
        
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(1, bounds.size.height-2, bounds.size.width-2, 1)); // Inner Bottom
        NSRectFill(NSMakeRect(bounds.size.width-2, 1, 1, bounds.size.height-2)); // Inner Right
    }
    
    NSMutableParagraphStyle *style = [[NSMutableParagraphStyle alloc] init];
    style.alignment = NSTextAlignmentCenter;
    NSDictionary *attrs = @{NSFontAttributeName: self.font ? self.font : [NSFont systemFontOfSize:12],
                            NSForegroundColorAttributeName: [NSColor blackColor],
                            NSParagraphStyleAttributeName: style};
    NSSize size = [self.title sizeWithAttributes:attrs];
    NSRect titleRect = NSMakeRect(0, (bounds.size.height - size.height) / 2.0, bounds.size.width, size.height);
    if (pressed) { titleRect.origin.x += 1; titleRect.origin.y += 1; }
    if (self.image) {
        NSSize imgSize = self.image.size;
        NSRect imgRect = NSMakeRect((bounds.size.width - imgSize.width)/2.0, (bounds.size.height - imgSize.height)/2.0, imgSize.width, imgSize.height);
        if (pressed) { imgRect.origin.x += 1; imgRect.origin.y += 1; }
        [self.image drawInRect:imgRect];
    } else {
        [self.title drawInRect:titleRect withAttributes:attrs];
    }
}
@end

@interface Win95TextField : NSTextField
@end
@implementation Win95TextField
- (BOOL)isFlipped { return YES; }
- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) { self.bordered = NO; self.drawsBackground = YES; self.backgroundColor = [NSColor whiteColor]; }
    return self;
}
- (void)drawRect:(NSRect)dirtyRect {
    NSRect bounds = self.bounds;
    [super drawRect:dirtyRect];
    // Sunken border
    [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set]; // Dark Gray outer top/left
    NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1));
    NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height));
    
    [[NSColor blackColor] set]; // Black inner top/left
    NSRectFill(NSMakeRect(1, 1, bounds.size.width-2, 1));
    NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height-2));
    
    [[NSColor whiteColor] set]; // White outer bottom/right
    NSRectFill(NSMakeRect(0, bounds.size.height-1, bounds.size.width, 1));
    NSRectFill(NSMakeRect(bounds.size.width-1, 0, 1, bounds.size.height));
    
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] set]; // Light Gray inner bottom/right
    NSRectFill(NSMakeRect(1, bounds.size.height-2, bounds.size.width-2, 1));
    NSRectFill(NSMakeRect(bounds.size.width-2, 1, 1, bounds.size.height-2));
}
@end

@interface Win95ScrollView : NSScrollView
@end
@implementation Win95ScrollView
- (BOOL)isFlipped { return YES; }
- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) { self.borderType = NSNoBorder; self.drawsBackground = YES; self.backgroundColor = [NSColor whiteColor]; }
    return self;
}
- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    NSRect bounds = self.bounds;
    // Sunken border
    [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set]; // Dark Gray outer top/left
    NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1));
    NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height));
    
    [[NSColor blackColor] set]; // Black inner top/left
    NSRectFill(NSMakeRect(1, 1, bounds.size.width-2, 1));
    NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height-2));
    
    [[NSColor whiteColor] set]; // White outer bottom/right
    NSRectFill(NSMakeRect(0, bounds.size.height-1, bounds.size.width, 1));
    NSRectFill(NSMakeRect(bounds.size.width-1, 0, 1, bounds.size.height));
    
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] set]; // Light Gray inner bottom/right
    NSRectFill(NSMakeRect(1, bounds.size.height-2, bounds.size.width-2, 1));
    NSRectFill(NSMakeRect(bounds.size.width-2, 1, 1, bounds.size.height-2));
}
@end
"""

start_str = "@interface Win95Button : NSButton"
end_str = "@implementation Win95ScrollView\n"

start_idx = content.find(start_str)
end_idx = content.find("@end", content.find(end_str)) + 4

new_content = content[:start_idx] + win95_classes_new + content[end_idx:]

with open("main.mm", "w") as f:
    f.write(new_content)

