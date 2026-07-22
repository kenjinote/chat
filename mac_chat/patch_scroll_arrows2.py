import re

with open("main.mm", "r") as f:
    content = f.read()

old_init = """        self.vUpBtn = [[Win95Button alloc] initWithFrame:NSZeroRect];
        self.vUpBtn.title = @"▲";
        [self addSubview:self.vUpBtn];
        
        self.vDownBtn = [[Win95Button alloc] initWithFrame:NSZeroRect];
        self.vDownBtn.title = @"▼";
        [self addSubview:self.vDownBtn];"""

new_init = """        self.vUpBtn = [[Win95Button alloc] initWithFrame:NSZeroRect];
        self.vUpBtn.title = @"▲";
        self.vUpBtn.target = self;
        self.vUpBtn.action = @selector(doScrollUp);
        [self addSubview:self.vUpBtn];
        
        self.vDownBtn = [[Win95Button alloc] initWithFrame:NSZeroRect];
        self.vDownBtn.title = @"▼";
        self.vDownBtn.target = self;
        self.vDownBtn.action = @selector(doScrollDown);
        [self addSubview:self.vDownBtn];"""

content = content.replace(old_init, new_init)

with open("main.mm", "w") as f:
    f.write(content)
