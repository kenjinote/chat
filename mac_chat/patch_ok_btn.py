import re

with open("main.mm", "r") as f:
    content = f.read()

old_interface = """@interface Win95ColorWell : NSControl
@property (nonatomic, strong) NSColor *color;
@property (nonatomic, assign) BOOL isPressed;
@end"""

new_interface = """@interface Win95ColorWell : NSControl
@property (nonatomic, strong) NSColor *color;
@property (nonatomic, assign) BOOL isPressed;
@property (nonatomic, strong) NSColor *originalColor;
@property (nonatomic, assign) BOOL confirmedColor;
@end"""
content = content.replace(old_interface, new_interface)

old_mousedown = """- (void)mouseDown:(NSEvent *)event {
    self.isPressed = YES;
    [self setNeedsDisplay:YES];
    
    NSColorPanel *panel = [NSColorPanel sharedColorPanel];
    panel.color = self.color;
    
    // Center it on the main window
    NSRect windowFrame = self.window.frame;
    NSRect panelFrame = panel.frame;
    panelFrame.origin.x = windowFrame.origin.x + (windowFrame.size.width - panelFrame.size.width) / 2.0;
    panelFrame.origin.y = windowFrame.origin.y + (windowFrame.size.height - panelFrame.size.height) / 2.0;
    [panel setFrame:panelFrame display:YES];
    
    id observer = [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowWillCloseNotification object:panel queue:nil usingBlock:^(NSNotification * _Nonnull note) {
        [NSApp stopModal];
    }];
    
    [NSApp runModalForWindow:panel];
    
    [[NSNotificationCenter defaultCenter] removeObserver:observer];
    
    self.color = panel.color;
    self.isPressed = NO;
    [self setNeedsDisplay:YES];
    
    // Trigger action
    if (self.action) {
        [NSApp sendAction:self.action to:self.target from:self];
    }
}"""

new_mousedown = """- (void)onColorPanelOK:(id)sender {
    self.confirmedColor = YES;
    [[NSColorPanel sharedColorPanel] close];
}

- (void)onColorPanelCancel:(id)sender {
    self.confirmedColor = NO;
    [[NSColorPanel sharedColorPanel] close];
}

- (void)mouseDown:(NSEvent *)event {
    self.isPressed = YES;
    [self setNeedsDisplay:YES];
    
    self.originalColor = self.color;
    self.confirmedColor = NO;
    
    NSColorPanel *panel = [NSColorPanel sharedColorPanel];
    panel.color = self.color;
    panel.showsAlpha = NO;
    
    // Setup OK / Cancel accessory view
    NSView *accView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, panel.frame.size.width, 40)];
    Win95Button *okBtn = [[Win95Button alloc] initWithFrame:NSMakeRect(accView.bounds.size.width - 160, 8, 70, 24)];
    okBtn.title = @"OK";
    okBtn.target = self;
    okBtn.action = @selector(onColorPanelOK:);
    [accView addSubview:okBtn];
    
    Win95Button *cancelBtn = [[Win95Button alloc] initWithFrame:NSMakeRect(accView.bounds.size.width - 80, 8, 70, 24)];
    cancelBtn.title = @"キャンセル";
    cancelBtn.target = self;
    cancelBtn.action = @selector(onColorPanelCancel:);
    [accView addSubview:cancelBtn];
    
    panel.accessoryView = accView;
    
    // Center it on the main window
    NSRect windowFrame = self.window.frame;
    NSRect panelFrame = panel.frame;
    panelFrame.origin.x = windowFrame.origin.x + (windowFrame.size.width - panelFrame.size.width) / 2.0;
    panelFrame.origin.y = windowFrame.origin.y + (windowFrame.size.height - panelFrame.size.height) / 2.0;
    [panel setFrame:panelFrame display:YES];
    
    id observer = [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowWillCloseNotification object:panel queue:nil usingBlock:^(NSNotification * _Nonnull note) {
        [NSApp stopModal];
    }];
    
    [NSApp runModalForWindow:panel];
    
    [[NSNotificationCenter defaultCenter] removeObserver:observer];
    panel.accessoryView = nil; // Clean up
    
    if (self.confirmedColor) {
        self.color = panel.color;
        if (self.action) {
            [NSApp sendAction:self.action to:self.target from:self];
        }
    } else {
        self.color = self.originalColor;
    }
    
    self.isPressed = NO;
    [self setNeedsDisplay:YES];
}"""

content = content.replace(old_mousedown, new_mousedown)

with open("main.mm", "w") as f:
    f.write(content)
