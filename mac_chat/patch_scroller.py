import re

with open("main.mm", "r") as f:
    content = f.read()

win95_scroller = """@interface Win95Scroller : NSScroller
@end

@implementation Win95Scroller
+ (BOOL)isCompatibleWithOverlayScrollers {
    return NO;
}
- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        self.scrollerStyle = NSScrollerStyleLegacy;
    }
    return self;
}
- (void)drawKnobSlotInRect:(NSRect)slotRect highlight:(BOOL)flag {
    // Checkerboard or light gray pattern. 
    // Win95 usually uses a checkered gray for the track, but light gray is acceptable and simpler.
    // Actually, drawing a checkerboard is very Win95!
    [[NSColor colorWithCalibratedWhite:0.88 alpha:1.0] setFill];
    NSRectFill(slotRect);
    
    // Draw a quick checkerboard pattern
    [[NSColor colorWithCalibratedWhite:0.95 alpha:1.0] setFill];
    NSBezierPath *path = [NSBezierPath bezierPath];
    for (int y = slotRect.origin.y; y < slotRect.origin.y + slotRect.size.height; y += 2) {
        for (int x = slotRect.origin.x + (y % 4 == 0 ? 0 : 2); x < slotRect.origin.x + slotRect.size.width; x += 4) {
            [path appendBezierPathWithRect:NSMakeRect(x, y, 2, 2)];
        }
    }
    [path fill];
}
- (void)drawKnob {
    NSRect knobRect = [self rectForPart:NSScrollerKnob];
    
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
    NSRectFill(knobRect);
    
    // Raised 3D effect
    // Top & Left
    [[NSColor whiteColor] set];
    NSRectFill(NSMakeRect(knobRect.origin.x, knobRect.origin.y, knobRect.size.width, 1));
    NSRectFill(NSMakeRect(knobRect.origin.x, knobRect.origin.y, 1, knobRect.size.height));
    
    // Bottom & Right
    [[NSColor blackColor] set];
    NSRectFill(NSMakeRect(knobRect.origin.x, knobRect.origin.y + knobRect.size.height - 1, knobRect.size.width, 1));
    NSRectFill(NSMakeRect(knobRect.origin.x + knobRect.size.width - 1, knobRect.origin.y, 1, knobRect.size.height));
    
    // Inner Top & Left
    [[NSColor colorWithCalibratedRed:0.85 green:0.85 blue:0.85 alpha:1.0] set];
    NSRectFill(NSMakeRect(knobRect.origin.x + 1, knobRect.origin.y + 1, knobRect.size.width - 2, 1));
    NSRectFill(NSMakeRect(knobRect.origin.x + 1, knobRect.origin.y + 1, 1, knobRect.size.height - 2));
    
    // Inner Bottom & Right
    [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
    NSRectFill(NSMakeRect(knobRect.origin.x + 1, knobRect.origin.y + knobRect.size.height - 2, knobRect.size.width - 2, 1));
    NSRectFill(NSMakeRect(knobRect.origin.x + knobRect.size.width - 2, knobRect.origin.y + 1, 1, knobRect.size.height - 2));
}

// Deprecated in recent macOS, but try drawing arrows if the OS calls it
- (void)drawArrow:(NSScrollerArrow)whichArrow highlight:(BOOL)flag {
    NSRect arrowRect = [self rectForPart:(whichArrow == NSScrollerDecrementArrow ? NSScrollerDecrementLine : NSScrollerIncrementLine)];
    if (NSIsEmptyRect(arrowRect)) return;
    
    if (flag) {
        [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
        NSRectFill(arrowRect);
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(arrowRect.origin.x, arrowRect.origin.y, arrowRect.size.width, 1));
        NSRectFill(NSMakeRect(arrowRect.origin.x, arrowRect.origin.y, 1, arrowRect.size.height));
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(arrowRect.origin.x+1, arrowRect.origin.y+1, arrowRect.size.width-2, 1));
        NSRectFill(NSMakeRect(arrowRect.origin.x+1, arrowRect.origin.y+1, 1, arrowRect.size.height-2));
    } else {
        [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
        NSRectFill(arrowRect);
        [[NSColor whiteColor] set];
        NSRectFill(NSMakeRect(arrowRect.origin.x, arrowRect.origin.y, arrowRect.size.width, 1));
        NSRectFill(NSMakeRect(arrowRect.origin.x, arrowRect.origin.y, 1, arrowRect.size.height));
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(arrowRect.origin.x, arrowRect.origin.y + arrowRect.size.height - 1, arrowRect.size.width, 1));
        NSRectFill(NSMakeRect(arrowRect.origin.x + arrowRect.size.width - 1, arrowRect.origin.y, 1, arrowRect.size.height));
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(arrowRect.origin.x+1, arrowRect.origin.y + arrowRect.size.height - 2, arrowRect.size.width-2, 1));
        NSRectFill(NSMakeRect(arrowRect.origin.x + arrowRect.size.width - 2, arrowRect.origin.y+1, 1, arrowRect.size.height-2));
    }
    
    [[NSColor blackColor] setFill];
    NSBezierPath *path = [NSBezierPath bezierPath];
    CGFloat cx = NSMidX(arrowRect);
    CGFloat cy = NSMidY(arrowRect);
    if (flag) { cx += 1; cy += 1; }
    
    if (self.bounds.size.width > self.bounds.size.height) { // Horizontal
        if (whichArrow == NSScrollerDecrementArrow) {
            [path moveToPoint:NSMakePoint(cx - 2, cy)];
            [path lineToPoint:NSMakePoint(cx + 2, cy - 3)];
            [path lineToPoint:NSMakePoint(cx + 2, cy + 3)];
        } else {
            [path moveToPoint:NSMakePoint(cx + 2, cy)];
            [path lineToPoint:NSMakePoint(cx - 2, cy - 3)];
            [path lineToPoint:NSMakePoint(cx - 2, cy + 3)];
        }
    } else { // Vertical
        if (whichArrow == NSScrollerDecrementArrow) { // Up
            [path moveToPoint:NSMakePoint(cx, cy - 2)];
            [path lineToPoint:NSMakePoint(cx - 3, cy + 2)];
            [path lineToPoint:NSMakePoint(cx + 3, cy + 2)];
        } else { // Down
            [path moveToPoint:NSMakePoint(cx, cy + 2)];
            [path lineToPoint:NSMakePoint(cx - 3, cy - 2)];
            [path lineToPoint:NSMakePoint(cx + 3, cy - 2)];
        }
    }
    [path fill];
}
@end
"""

# Insert Win95Scroller definition before Win95ScrollView
insert_idx = content.find("@interface Win95ScrollView")
content = content[:insert_idx] + win95_scroller + "\n" + content[insert_idx:]

# Update Win95ScrollView to use Win95Scroller
old_scrollview_init = """- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) { self.borderType = NSNoBorder; self.drawsBackground = YES; self.backgroundColor = [NSColor whiteColor]; }
    return self;
}"""
new_scrollview_init = """- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) { 
        self.borderType = NSNoBorder; 
        self.drawsBackground = YES; 
        self.backgroundColor = [NSColor whiteColor]; 
        
        Win95Scroller *vScroller = [[Win95Scroller alloc] init];
        self.verticalScroller = vScroller;
        Win95Scroller *hScroller = [[Win95Scroller alloc] init];
        self.horizontalScroller = hScroller;
    }
    return self;
}"""
content = content.replace(old_scrollview_init, new_scrollview_init)

with open("main.mm", "w") as f:
    f.write(content)
