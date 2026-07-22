import re

with open("main.mm", "r") as f:
    content = f.read()

# 1. Update interface
old_interface = """@interface Win95Button : NSButton
@property (assign) BOOL isCheckbox;
@property (assign) BOOL isGreenWhenPressed;
@end"""
new_interface = """@interface Win95Button : NSButton
@property (assign) BOOL isCheckbox;
@property (assign) BOOL isGreenWhenPressed;
@property (assign) BOOL isToggleButton;
@property (assign) BOOL hasMicIcon;
@end"""
content = content.replace(old_interface, new_interface)

# 2. Update setButtonType
old_set_btn = """- (void)setButtonType:(NSButtonType)type {
    [super setButtonType:type];
    if (type == NSButtonTypeSwitch || type == NSButtonTypeRadio) {
        self.isCheckbox = YES;
    }
}"""
new_set_btn = """- (void)setButtonType:(NSButtonType)type {
    [super setButtonType:type];
    if (type == NSButtonTypeSwitch || type == NSButtonTypeRadio) {
        self.isCheckbox = YES;
        self.isToggleButton = YES;
    } else if (type == NSButtonTypePushOnPushOff) {
        self.isToggleButton = YES;
    } else {
        self.isToggleButton = NO;
    }
}"""
content = content.replace(old_set_btn, new_set_btn)

# 3. Update drawRect pressed logic
old_pressed = """    BOOL pressed = self.isHighlighted || self.state == NSControlStateValueOn;"""
new_pressed = """    BOOL pressed = self.isHighlighted;
    if (self.isToggleButton) {
        pressed = pressed || (self.state == NSControlStateValueOn);
    }"""
content = content.replace(old_pressed, new_pressed)

# 4. Add mic drawing logic inside drawRect before title
old_draw_title = """    if (self.image) {
        NSSize imgSize = self.image.size;
        NSRect imgRect = NSMakeRect((bounds.size.width - imgSize.width)/2.0, (bounds.size.height - imgSize.height)/2.0, imgSize.width, imgSize.height);
        if (pressed) { imgRect.origin.x += 1; imgRect.origin.y += 1; }
        [self.image drawInRect:imgRect];
    } else {
        [self.title drawInRect:titleRect withAttributes:attrs];
    }"""

new_draw_title = """    if (self.hasMicIcon) {
        CGFloat cx = bounds.size.width / 2.0 - 18;
        CGFloat cy = bounds.size.height / 2.0;
        if (pressed) { cx += 1; cy += 1; }
        
        [[NSColor blackColor] setStroke];
        [[NSColor colorWithCalibratedRed:0.2 green:0.2 blue:0.2 alpha:1.0] setFill];
        
        // mic head (capsule)
        NSRect capRect = NSMakeRect(cx - 3, cy - 5, 6, 10);
        NSBezierPath *cap = [NSBezierPath bezierPathWithRoundedRect:capRect xRadius:2 yRadius:2];
        [cap fill];
        
        // U shape
        NSBezierPath *u = [NSBezierPath bezierPath];
        [u moveToPoint:NSMakePoint(cx - 5, cy)];
        [u lineToPoint:NSMakePoint(cx - 5, cy + 5)];
        [u lineToPoint:NSMakePoint(cx + 5, cy + 5)];
        [u lineToPoint:NSMakePoint(cx + 5, cy)];
        [u setLineWidth:1.5];
        [u stroke];
        
        // stem and base
        NSBezierPath *stem = [NSBezierPath bezierPath];
        [stem moveToPoint:NSMakePoint(cx, cy + 5)];
        [stem lineToPoint:NSMakePoint(cx, cy + 9)];
        [stem moveToPoint:NSMakePoint(cx - 4, cy + 9)];
        [stem lineToPoint:NSMakePoint(cx + 4, cy + 9)];
        [stem setLineWidth:1.5];
        [stem stroke];
        
        titleRect.origin.x += 8;
    }

    if (self.image) {
        NSSize imgSize = self.image.size;
        NSRect imgRect = NSMakeRect((bounds.size.width - imgSize.width)/2.0, (bounds.size.height - imgSize.height)/2.0, imgSize.width, imgSize.height);
        if (pressed) { imgRect.origin.x += 1; imgRect.origin.y += 1; }
        [self.image drawInRect:imgRect];
    } else {
        [self.title drawInRect:titleRect withAttributes:attrs];
    }"""
content = content.replace(old_draw_title, new_draw_title)

# 5. Set hasMicIcon for voiceButton
old_voice = """    ((Win95Button*)self.voiceButton).isGreenWhenPressed = YES;"""
new_voice = """    ((Win95Button*)self.voiceButton).isGreenWhenPressed = YES;
    ((Win95Button*)self.voiceButton).hasMicIcon = YES;"""
content = content.replace(old_voice, new_voice)

with open("main.mm", "w") as f:
    f.write(content)
