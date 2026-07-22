import re

with open("main.mm", "r") as f:
    content = f.read()

old_init = """- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        self.bordered = NO;
        self.backgroundColor = [NSColor whiteColor];
        self.drawsBackground = YES;
    }
    return self;
}"""

new_init = """- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        self.bordered = NO;
        self.backgroundColor = [NSColor whiteColor];
        self.drawsBackground = YES;
        self.focusRingType = NSFocusRingTypeNone;
    }
    return self;
}"""

content = content.replace(old_init, new_init)

with open("main.mm", "w") as f:
    f.write(content)

