import re

with open("main.mm", "r") as f:
    content = f.read()

# 1. Update Win95Button interface
old_interface = """@interface Win95Button : NSButton
@property (assign) BOOL isCheckbox;
@property (assign) BOOL isGreenWhenPressed;
@property (assign) BOOL isToggleButton;
@property (assign) BOOL hasMicIcon;
@end"""

new_interface = """typedef NS_ENUM(NSInteger, Win95ArrowType) {
    Win95ArrowNone = 0,
    Win95ArrowUp,
    Win95ArrowDown
};

@interface Win95Button : NSButton
@property (assign) BOOL isCheckbox;
@property (assign) BOOL isGreenWhenPressed;
@property (assign) BOOL isToggleButton;
@property (assign) BOOL hasMicIcon;
@property (assign) Win95ArrowType arrowType;
@end"""
content = content.replace(old_interface, new_interface)

# 2. Update drawRect to draw the arrows instead of title
old_draw_title = """    if (self.image) {
        NSSize imgSize = self.image.size;
        NSRect imgRect = NSMakeRect((bounds.size.width - imgSize.width)/2.0, (bounds.size.height - imgSize.height)/2.0, imgSize.width, imgSize.height);
        if (pressed) { imgRect.origin.x += 1; imgRect.origin.y += 1; }
        [self.image drawInRect:imgRect];
    } else {
        [self.title drawInRect:titleRect withAttributes:attrs];
    }"""

new_draw_title = """    if (self.arrowType != Win95ArrowNone) {
        [[NSColor blackColor] setFill];
        CGFloat cx = round(bounds.size.width / 2.0);
        CGFloat cy = round(bounds.size.height / 2.0);
        if (pressed) { cx += 1; cy += 1; }
        
        if (self.arrowType == Win95ArrowUp) {
            NSRectFill(NSMakeRect(cx, cy - 2, 1, 1));
            NSRectFill(NSMakeRect(cx - 1, cy - 1, 3, 1));
            NSRectFill(NSMakeRect(cx - 2, cy, 5, 1));
            NSRectFill(NSMakeRect(cx - 3, cy + 1, 7, 1));
        } else if (self.arrowType == Win95ArrowDown) {
            NSRectFill(NSMakeRect(cx - 3, cy - 1, 7, 1));
            NSRectFill(NSMakeRect(cx - 2, cy, 5, 1));
            NSRectFill(NSMakeRect(cx - 1, cy + 1, 3, 1));
            NSRectFill(NSMakeRect(cx, cy + 2, 1, 1));
        }
    } else if (self.image) {
        NSSize imgSize = self.image.size;
        NSRect imgRect = NSMakeRect((bounds.size.width - imgSize.width)/2.0, (bounds.size.height - imgSize.height)/2.0, imgSize.width, imgSize.height);
        if (pressed) { imgRect.origin.x += 1; imgRect.origin.y += 1; }
        [self.image drawInRect:imgRect];
    } else {
        [self.title drawInRect:titleRect withAttributes:attrs];
    }"""
content = content.replace(old_draw_title, new_draw_title)

# 3. Update Win95ScrollView to use arrowType instead of text
old_up_init = """        self.vUpBtn = [[Win95Button alloc] initWithFrame:NSZeroRect];
        self.vUpBtn.title = @"▲";
        self.vUpBtn.target = self;
        self.vUpBtn.action = @selector(doScrollUp);"""

new_up_init = """        self.vUpBtn = [[Win95Button alloc] initWithFrame:NSZeroRect];
        self.vUpBtn.title = @"";
        self.vUpBtn.arrowType = Win95ArrowUp;
        self.vUpBtn.target = self;
        self.vUpBtn.action = @selector(doScrollUp);"""
content = content.replace(old_up_init, new_up_init)

old_down_init = """        self.vDownBtn = [[Win95Button alloc] initWithFrame:NSZeroRect];
        self.vDownBtn.title = @"▼";
        self.vDownBtn.target = self;
        self.vDownBtn.action = @selector(doScrollDown);"""

new_down_init = """        self.vDownBtn = [[Win95Button alloc] initWithFrame:NSZeroRect];
        self.vDownBtn.title = @"";
        self.vDownBtn.arrowType = Win95ArrowDown;
        self.vDownBtn.target = self;
        self.vDownBtn.action = @selector(doScrollDown);"""
content = content.replace(old_down_init, new_down_init)

with open("main.mm", "w") as f:
    f.write(content)
