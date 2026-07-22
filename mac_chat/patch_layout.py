import re

with open("main.mm", "r") as f:
    content = f.read()

# Add edge view properties
if "@property (strong) Win95EdgeView *logEdge;" not in content:
    content = content.replace("@property (strong) NSScrollView *logScroll;", 
                              "@property (strong) Win95EdgeView *logEdge;\n@property (strong) NSScrollView *logScroll;")
    content = content.replace("@property (strong) NSScrollView *inputScroll;",
                              "@property (strong) Win95EdgeView *inputEdge;\n@property (strong) NSScrollView *inputScroll;")
    content = content.replace("@property (strong) NSScrollView *userScroll;",
                              "@property (strong) Win95EdgeView *userEdge;\n@property (strong) NSScrollView *userScroll;")

# Initialize and wrap in initializeUI
init_log = """    self.logScroll = [[Win95ScrollView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];"""
init_log_new = """    self.logEdge = [[Win95EdgeView alloc] initWithFrame:NSZeroRect];
    [self.window.contentView addSubview:self.logEdge];
    self.logScroll = [[Win95ScrollView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    [self.logEdge addSubview:self.logScroll];"""
content = content.replace(init_log, init_log_new)

init_user = """    self.userScroll = [[Win95ScrollView alloc] initWithFrame:NSZeroRect];"""
init_user_new = """    self.userEdge = [[Win95EdgeView alloc] initWithFrame:NSZeroRect];
    [self.window.contentView addSubview:self.userEdge];
    self.userScroll = [[Win95ScrollView alloc] initWithFrame:NSZeroRect];
    [self.userEdge addSubview:self.userScroll];"""
content = content.replace(init_user, init_user_new)

init_input = """    self.inputScroll = [[Win95ScrollView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];"""
init_input_new = """    self.inputEdge = [[Win95EdgeView alloc] initWithFrame:NSZeroRect];
    [self.window.contentView addSubview:self.inputEdge];
    self.inputScroll = [[Win95ScrollView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    [self.inputEdge addSubview:self.inputScroll];"""
content = content.replace(init_input, init_input_new)

# Remove old addSubviews for scrolls
content = content.replace("[self.window.contentView addSubview:self.logScroll];", "")
content = content.replace("[self.window.contentView addSubview:self.userScroll];", "")
content = content.replace("[self.window.contentView addSubview:self.inputScroll];", "")

# Update resizeViews
old_resize = """    self.logScroll.frame = NSMakeRect(padding, padding + bottomPanelHeight + toolBarHeight,
                                      contentSize.width - rightPanelWidth - gap - padding*2,
                                      contentSize.height - bottomPanelHeight - toolBarHeight - padding*2);
    
    self.userScroll.frame = NSMakeRect(contentSize.width - rightPanelWidth - padding,
                                       padding + bottomPanelHeight + toolBarHeight,
                                       rightPanelWidth,
                                       contentSize.height - bottomPanelHeight - toolBarHeight - padding*2 - userLabelHeight);
    
    // Bottom panel
    self.inputScroll.frame = NSMakeRect(padding, padding + 4,
                                        contentSize.width - btnWidth - gap - padding*2,
                                        inputHeight);"""

new_resize = """    NSRect logFrame = NSMakeRect(padding, padding + bottomPanelHeight + toolBarHeight,
                                      contentSize.width - rightPanelWidth - gap - padding*2,
                                      contentSize.height - bottomPanelHeight - toolBarHeight - padding*2);
    self.logEdge.frame = logFrame;
    self.logScroll.frame = NSMakeRect(2, 2, logFrame.size.width - 4, logFrame.size.height - 4);
    
    NSRect userFrame = NSMakeRect(contentSize.width - rightPanelWidth - padding,
                                       padding + bottomPanelHeight + toolBarHeight,
                                       rightPanelWidth,
                                       contentSize.height - bottomPanelHeight - toolBarHeight - padding*2 - userLabelHeight);
    self.userEdge.frame = userFrame;
    self.userScroll.frame = NSMakeRect(2, 2, userFrame.size.width - 4, userFrame.size.height - 4);
    
    // Bottom panel
    NSRect inputFrame = NSMakeRect(padding, padding + 4,
                                        contentSize.width - btnWidth - gap - padding*2,
                                        inputHeight);
    self.inputEdge.frame = inputFrame;
    self.inputScroll.frame = NSMakeRect(2, 2, inputFrame.size.width - 4, inputFrame.size.height - 4);"""
content = content.replace(old_resize, new_resize)

with open("main.mm", "w") as f:
    f.write(content)
