import re

with open("main.mm", "r") as f:
    content = f.read()

content = re.sub(
r"""    self\.voiceButton\.action = @selector\(onVoiceClicked:\);
    \[self\.window\.contentView addSubview:self\.voiceButton\];
        \}
    \}\];
    \[task resume\];
    self\.micVolumeIndicator = \[\[NSLevelIndicator alloc\] initWithFrame:NSZeroRect\];""",
r"""    self.voiceButton.action = @selector(onVoiceClicked:);
    [self.window.contentView addSubview:self.voiceButton];
    self.micVolumeIndicator = [[NSLevelIndicator alloc] initWithFrame:NSZeroRect];""", content)

content = re.sub(
r"""    self\.volLabel = \[\[NSTextField alloc\] initWithFrame:NSZeroRect\];
    self\.logScroll\.frame = NSMakeRect\(5, 100, bounds\.size\.width - 165, bounds\.size\.height - 150\);
    self\.userScroll\.frame = NSMakeRect\(bounds\.size\.width - 155, 50, 150, bounds\.size\.height - 100\);
    self\.inputScroll\.frame = NSMakeRect\(5, 5, bounds\.size\.width - 80, 40\);
    self\.sendButton\.frame = NSMakeRect\(bounds\.size\.width - 70, 5, 65, 40\);
\}

    self\.volLabel\.bordered = NO;""",
r"""    self.volLabel = [[NSTextField alloc] initWithFrame:NSZeroRect];
    self.volLabel.bordered = NO;""", content)

with open("main.mm", "w") as f:
    f.write(content)
