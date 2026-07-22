import re

with open("main.mm", "r") as f:
    content = f.read()

# Replace Win95ColorWell interface and implementation
old_impl = """@interface Win95ColorWell : NSColorWell
@end

@implementation Win95ColorWell
- (BOOL)isFlipped { return YES; }

- (void)drawRect:(NSRect)dirtyRect {
    NSRect bounds = self.bounds;
    BOOL pressed = [self isActive]; // active means color panel is open"""

new_impl = """@interface Win95ColorWell : NSControl
@property (nonatomic, strong) NSColor *color;
@property (nonatomic, assign) BOOL isPressed;
@end

@implementation Win95ColorWell

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        _color = [NSColor blackColor]; // Default color
    }
    return self;
}

- (void)setColor:(NSColor *)color {
    if (_color != color) {
        _color = color;
        [self setNeedsDisplay:YES];
    }
}

- (BOOL)isFlipped { return YES; }

- (void)mouseDown:(NSEvent *)event {
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
}

- (void)drawRect:(NSRect)dirtyRect {
    NSRect bounds = self.bounds;
    BOOL pressed = self.isPressed;"""

content = content.replace(old_impl, new_impl)

# Change property type in MainViewController
old_prop = """@property (strong) NSColorWell *colorWell;"""
new_prop = """@property (strong) Win95ColorWell *colorWell;"""
content = content.replace(old_prop, new_prop)

# Fix local var type in alerts
old_local = """NSColorWell *colorWell = [[Win95ColorWell alloc] initWithFrame:NSMakeRect(100, 0, 50, 24)];"""
new_local = """Win95ColorWell *colorWell = [[Win95ColorWell alloc] initWithFrame:NSMakeRect(100, 0, 50, 24)];"""
content = content.replace(old_local, new_local)

with open("main.mm", "w") as f:
    f.write(content)
