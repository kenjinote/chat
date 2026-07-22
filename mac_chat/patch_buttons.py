import re

with open("main.mm", "r") as f:
    content = f.read()

# 1. Update Win95Button class
old_win95_interface = """@interface Win95Button : NSButton
@property (assign) BOOL isCheckbox;
@end"""
new_win95_interface = """@interface Win95Button : NSButton
@property (assign) BOOL isCheckbox;
@property (assign) BOOL isGreenWhenPressed;
@end"""
content = content.replace(old_win95_interface, new_win95_interface)

old_setButtonType = """- (void)setButtonType:(NSButtonType)type {
    [super setButtonType:type];
    if (type == NSButtonTypeSwitch || type == NSButtonTypeRadio) {
        self.isCheckbox = YES;
    }
}"""
new_setButtonType = """- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        [self setButtonType:NSButtonTypeMomentaryPushIn];
    }
    return self;
}

- (void)setButtonType:(NSButtonType)type {
    [super setButtonType:type];
    if (type == NSButtonTypeSwitch || type == NSButtonTypeRadio) {
        self.isCheckbox = YES;
    }
}"""
content = content.replace(old_setButtonType, new_setButtonType)

# Update drawRect: to handle green background
old_draw_rect_bg = """    NSRect bounds = self.bounds;
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
    NSRectFill(bounds);
    
    BOOL pressed = self.isHighlighted || self.state == NSControlStateValueOn;"""
new_draw_rect_bg = """    NSRect bounds = self.bounds;
    
    BOOL pressed = self.isHighlighted || self.state == NSControlStateValueOn;
    
    if (pressed && self.isGreenWhenPressed) {
        [[NSColor colorWithCalibratedRed:0.0 green:0.7 blue:0.0 alpha:1.0] setFill];
    } else {
        [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
    }
    NSRectFill(bounds);"""
content = content.replace(old_draw_rect_bg, new_draw_rect_bg)


# 2. Modify voiceButton
voice_setup_old = """    self.voiceButton = [[Win95Button alloc] initWithFrame:NSZeroRect];
    self.voiceButton.title = [NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_TALK", nil)];"""
voice_setup_new = """    self.voiceButton = [[Win95Button alloc] initWithFrame:NSZeroRect];
    [self.voiceButton setButtonType:NSButtonTypePushOnPushOff];
    ((Win95Button*)self.voiceButton).isGreenWhenPressed = YES;
    self.voiceButton.title = [NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_TALK", nil)];"""
content = content.replace(voice_setup_old, voice_setup_new)

# 3. Remove PushOnPushOff from btnB, btnI, btnU
content = content.replace("    [self.btnB setButtonType:NSButtonTypePushOnPushOff];\n", "")
content = content.replace("    [self.btnI setButtonType:NSButtonTypePushOnPushOff];\n", "")
content = content.replace("    [self.btnU setButtonType:NSButtonTypePushOnPushOff];\n", "")

with open("main.mm", "w") as f:
    f.write(content)

