import re

with open("main.mm", "r") as f:
    content = f.read()

# Replace Win95ScrollView definition with one that includes tile and buttons
old_scroll_view = """@interface Win95ScrollView : NSScrollView
@end
@implementation Win95ScrollView
- (BOOL)isFlipped { return YES; }
- (instancetype)initWithFrame:(NSRect)frameRect {
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
@end"""

new_scroll_view = """@interface Win95ScrollView : NSScrollView
@property (strong) Win95Button *vUpBtn;
@property (strong) Win95Button *vDownBtn;
@property (strong) NSTimer *scrollTimer;
@end
@implementation Win95ScrollView
- (BOOL)isFlipped { return YES; }
- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) { 
        self.borderType = NSNoBorder; 
        self.drawsBackground = YES; 
        self.backgroundColor = [NSColor whiteColor]; 
        
        Win95Scroller *vScroller = [[Win95Scroller alloc] init];
        self.verticalScroller = vScroller;
        
        self.vUpBtn = [[Win95Button alloc] initWithFrame:NSZeroRect];
        self.vUpBtn.title = @"▲";
        [self addSubview:self.vUpBtn];
        
        self.vDownBtn = [[Win95Button alloc] initWithFrame:NSZeroRect];
        self.vDownBtn.title = @"▼";
        [self addSubview:self.vDownBtn];
    }
    return self;
}

- (void)tile {
    [super tile];
    if (self.hasVerticalScroller && self.verticalScroller) {
        NSRect vsFrame = self.verticalScroller.frame;
        CGFloat btnHeight = vsFrame.size.width; // square button
        
        self.vUpBtn.frame = NSMakeRect(vsFrame.origin.x, vsFrame.origin.y, vsFrame.size.width, btnHeight);
        self.vDownBtn.frame = NSMakeRect(vsFrame.origin.x, vsFrame.origin.y + vsFrame.size.height - btnHeight, vsFrame.size.width, btnHeight);
        
        vsFrame.origin.y += btnHeight;
        vsFrame.size.height -= btnHeight * 2;
        self.verticalScroller.frame = vsFrame;
        
        self.vUpBtn.hidden = NO;
        self.vDownBtn.hidden = NO;
    } else {
        self.vUpBtn.hidden = YES;
        self.vDownBtn.hidden = YES;
    }
}

- (void)doScrollUp {
    NSPoint pt = self.contentView.bounds.origin;
    pt.y -= 20;
    if (pt.y < 0) pt.y = 0;
    [self.contentView scrollToPoint:pt];
    [self reflectScrolledClipView:self.contentView];
}

- (void)doScrollDown {
    NSPoint pt = self.contentView.bounds.origin;
    pt.y += 20;
    NSRect docRect = [self.documentView frame];
    NSRect clipRect = [self.contentView bounds];
    if (pt.y > docRect.size.height - clipRect.size.height) pt.y = docRect.size.height - clipRect.size.height;
    if (pt.y < 0) pt.y = 0;
    [self.contentView scrollToPoint:pt];
    [self reflectScrolledClipView:self.contentView];
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
@end"""

content = content.replace(old_scroll_view, new_scroll_view)

with open("main.mm", "w") as f:
    f.write(content)
