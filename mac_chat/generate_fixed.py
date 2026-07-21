import re

with open("main.mm", "r") as f:
    lines = f.readlines()

new_lines = []
for i in range(882):  # Keep up to line 882 (where self.inputTextView is setup)
    new_lines.append(lines[i])

# Now append the rest of applicationDidFinishLaunching:
new_lines.append("""    self.inputTextView.delegate = self;
    self.inputScroll.documentView = self.inputTextView;
    [self.window.contentView addSubview:self.inputScroll];
    
    self.sendButton = [[NSButton alloc] initWithFrame:NSZeroRect];
    self.sendButton.title = NSLocalizedString(@"IDS_SEND", nil);
    self.sendButton.target = self;
    self.sendButton.action = @selector(onSendClicked:);
    [self.window.contentView addSubview:self.sendButton];
    
    [self layoutControls];
    [self.window makeKeyAndOrderFront:nil];
    [self onFormatChanged:nil];
    
    if (chatNetwork->Connect("chat-lb-2026.japaneast.cloudapp.azure.com", 4000, 4001)) {
        NSString *joinMsg = [NSString stringWithFormat:@"{\\"type\\":\\"join\\", \\"secretId\\":\\"%s\\", \\"userName\\":\\"%s\\", \\"roomName\\":\\"Lobby\\", \\"pub\\":true, \\"nclr\\":0}\\n",
                             g_secretId.c_str(), g_localUserName.c_str()];
        chatNetwork->SendMessage([joinMsg UTF8String]);
        chatNetwork->SendMessage("{\\"type\\":\\"get_rooms\\"}\\n");
    }
}

- (void)layoutControls {
    if (!self.window) return;
    NSRect bounds = self.window.contentView.bounds;
    self.logScroll.frame = NSMakeRect(5, 100, bounds.size.width - 165, bounds.size.height - 150);
    self.userScroll.frame = NSMakeRect(bounds.size.width - 155, 50, 150, bounds.size.height - 100);
    self.inputScroll.frame = NSMakeRect(5, 5, bounds.size.width - 80, 40);
    self.sendButton.frame = NSMakeRect(bounds.size.width - 70, 5, 65, 40);
}

- (void)updateWindowTitle {
    NSString *title = [NSString stringWithFormat:@"MacChat - [%@]", self.currentRoomName];
    [self.window setTitle:title];
}

- (void)onFormatChanged:(id)sender {
    NSFontTraitMask traits = 0;
    if (self.btnB.state == NSControlStateValueOn) traits |= NSBoldFontMask;
    if (self.btnI.state == NSControlStateValueOn) traits |= NSItalicFontMask;
    if (self.btnU.state == NSControlStateValueOn) {} // Handle underline separately if needed
    
    int fontSize = 12;
    NSInteger idx = self.sizeCombo.indexOfSelectedItem;
    if (idx >= 0 && idx < self.sizeCombo.numberOfItems) {
        fontSize = [[self.sizeCombo itemObjectValueAtIndex:idx] intValue];
    } else {
        fontSize = self.sizeCombo.intValue;
    }
    if (fontSize <= 0) fontSize = 12;
    
    NSFont *font = [[NSFontManager sharedFontManager] fontWithFamily:@"Helvetica" traits:traits weight:5 size:fontSize];
    NSDictionary *attrs = @{NSFontAttributeName: font, NSForegroundColorAttributeName: self.colorWell.color};
    self.inputTextView.typingAttributes = attrs;
}

- (void)appendLogMessage:(NSString *)message {
    [[self.logTextView textStorage] appendAttributedString:[[NSAttributedString alloc] initWithString:[message stringByAppendingString:@"\\n"]]];
}

- (void)appendSystemLogMessage:(NSString *)message {
    [self appendLogMessage:message];
}

- (void)onSendClicked:(id)sender {
    NSString *msg = self.inputTextView.string;
    if (msg.length > 0 && chatNetwork) {
        std::string json = "{\\"type\\":\\"chat\\", \\"msg\\":\\"" + [msg UTF8String] + "\\", \\"nclr\\":0}\\n";
        chatNetwork->SendMessage(json);
        self.inputTextView.string = @"";
    }
}

- (void)onVoiceClicked:(id)sender {
    if ([self.voiceButton.title isEqualToString:[NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_TALK", nil)]]) {
        self.voiceButton.title = [NSString stringWithFormat:@"%@: ON", NSLocalizedString(@"IDS_TALK", nil)];
        if (audioEngine) audioEngine->StartMicrophone();
    } else {
        self.voiceButton.title = [NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_TALK", nil)];
        if (audioEngine) audioEngine->StopMicrophone();
        self.micVolumeIndicator.doubleValue = 0;
    }
}

- (void)onVolumeSliderChanged:(id)sender {
    if (audioEngine) {
        audioEngine->SetPlaybackVolume(self.volumeSlider.floatValue);
    }
}

- (void)showLocalVideoWindowWithTitleSuffix:(NSString *)suffix {
    std::string uname = g_localUserName;
    NSString *rawName = [NSString stringWithUTF8String:uname.c_str()];
    NSRange hashRange = [rawName rangeOfString:@"#"];
    NSString *dispName = (hashRange.location != NSNotFound) ? [rawName substringToIndex:hashRange.location] : rawName;
    
    if (suffix) {
        dispName = [dispName stringByAppendingString:suffix];
    }
    
    if (g_videoWindows.find(g_userId) == g_videoWindows.end()) {
        VideoWindowController *videoWin = [[VideoWindowController alloc] initWithUserId:[NSString stringWithUTF8String:g_userId.c_str()] userName:dispName];
        [videoWin showWindow:self];
        videoWin.window.title = dispName;
        g_videoWindows[g_userId] = videoWin;
    } else {
        [g_videoWindows[g_userId] showWindow:self];
        [g_videoWindows[g_userId].window makeKeyAndOrderFront:self];
        g_videoWindows[g_userId].window.title = dispName;
    }
}

- (void)onVideoClicked:(id)sender {
    if (g_captureSourceType == 1) {
        g_captureSourceType = 0;
        self.videoButton.title = [NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_VIDEO_STREAM", nil)];
        if (videoEngine) videoEngine->StopCapture();
        if (g_videoWindows.find(g_userId) != g_videoWindows.end()) {
            [g_videoWindows[g_userId] close];
            g_videoWindows.erase(g_userId);
        }
    } else {
        g_captureSourceType = 1;
        self.videoButton.title = [NSString stringWithFormat:@"%@: ON", NSLocalizedString(@"IDS_VIDEO_STREAM", nil)];
        [self showLocalVideoWindowWithTitleSuffix:nil];
        if (videoEngine) videoEngine->StartCapture();
    }
}

- (void)onScreenContextClicked:(id)sender {
    if (g_captureSourceType == 2) {
        g_captureSourceType = 0;
        if (videoEngine) videoEngine->StopScreenCapture();
        if (g_videoWindows.find(g_userId) != g_videoWindows.end()) {
            [g_videoWindows[g_userId] close];
            g_videoWindows.erase(g_userId);
        }
    } else {
        g_captureSourceType = 2;
        self.videoButton.title = [NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_VIDEO_STREAM", nil)];
        [self showLocalVideoWindowWithTitleSuffix:@" (Screen)"];
        if (videoEngine) videoEngine->StartScreenCapture();
    }
}

- (void)onWindowShareClicked:(id)sender {
    NSMenuItem *item = (NSMenuItem*)sender;
    NSNumber *winId = item.representedObject;
    if (g_captureSourceType == 3 && g_captureWindowId == winId.unsignedIntValue) {
        g_captureSourceType = 0;
        if (videoEngine) videoEngine->StopWindowCapture();
        if (g_videoWindows.find(g_userId) != g_videoWindows.end()) {
            [g_videoWindows[g_userId] close];
            g_videoWindows.erase(g_userId);
        }
    } else {
        g_captureSourceType = 3;
        g_captureWindowId = winId.unsignedIntValue;
        self.videoButton.title = [NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_VIDEO_STREAM", nil)];
        [self showLocalVideoWindowWithTitleSuffix:@" (Window)"];
        if (videoEngine) videoEngine->StartWindowCapture(g_captureWindowId);
    }
}

- (void)menuNeedsUpdate:(NSMenu *)menu {
    [menu removeAllItems];
    NSInteger row = [self.userListView clickedRow];
    if (row >= 0 && row < g_userList.size()) {
        std::string uid = g_userList[row].userId;
        if (uid == g_userId) {
            NSMenuItem *camItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"IDS_VIDEO_STREAM", nil) action:@selector(onVideoClicked:) keyEquivalent:@""];
            [camItem setState:(g_captureSourceType == 1) ? NSControlStateValueOn : NSControlStateValueOff];
            [menu addItem:camItem];
            
            NSMenuItem *shareItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"IDS_SCREEN_SHARE", nil) action:nil keyEquivalent:@""];
            NSMenu *shareMenu = [[NSMenu alloc] initWithTitle:NSLocalizedString(@"IDS_SCREEN_SHARE", nil)];
            
            NSMenuItem *fullScreenItem = [[NSMenuItem alloc] initWithTitle:@"Display 1" action:@selector(onScreenContextClicked:) keyEquivalent:@""];
            [fullScreenItem setState:(g_captureSourceType == 2) ? NSControlStateValueOn : NSControlStateValueOff];
            [shareMenu addItem:fullScreenItem];
            
            [shareMenu addItem:[NSMenuItem separatorItem]];
            
            CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
            if (windowList) {
                int winCount = 0;
                for (int i = 0; i < CFArrayGetCount(windowList); i++) {
                    CFDictionaryRef dict = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);
                    NSString *owner = (__bridge NSString *)CFDictionaryGetValue(dict, kCGWindowOwnerName);
                    NSString *title = (__bridge NSString *)CFDictionaryGetValue(dict, kCGWindowName);
                    NSNumber *layer = (__bridge NSNumber *)CFDictionaryGetValue(dict, kCGWindowLayer);
                    
                    if (layer.intValue == 0 && owner.length > 0) {
                        if ([owner isEqualToString:@"MacChat"] || [owner isEqualToString:@"Terminal"]) continue;
                        if (!title) title = @"";
                        
                        NSString *menuTitle = [NSString stringWithFormat:@"[%@] %@", owner, title.length > 0 ? title : @"(No Title)"];
                        NSNumber *winId = (__bridge NSNumber *)CFDictionaryGetValue(dict, kCGWindowNumber);
                        
                        NSMenuItem *winItem = [[NSMenuItem alloc] initWithTitle:menuTitle action:@selector(onWindowShareClicked:) keyEquivalent:@""];
                        winItem.representedObject = winId;
                        [winItem setState:(g_captureSourceType == 3 && g_captureWindowId == winId.unsignedIntValue) ? NSControlStateValueOn : NSControlStateValueOff];
                        [shareMenu addItem:winItem];
                        
                        if (++winCount >= 50) break;
                    }
                }
                CFRelease(windowList);
            }
            
            [shareItem setSubmenu:shareMenu];
            [menu addItem:shareItem];
        } else {
            [menu addItemWithTitle:NSLocalizedString(@"IDS_PROFILE", nil) action:nil keyEquivalent:@""];
            [menu addItemWithTitle:NSLocalizedString(@"IDS_SEND_MESSAGE", nil) action:nil keyEquivalent:@""];
            if (g_ignoredUsers.find(uid) == g_ignoredUsers.end()) {
                [menu addItemWithTitle:NSLocalizedString(@"IDS_IGNORE", nil) action:@selector(onBlockClicked:) keyEquivalent:@""];
            } else {
                [menu addItemWithTitle:NSLocalizedString(@"IDS_UNIGNORE", nil) action:@selector(onBlockClicked:) keyEquivalent:@""];
            }
            [menu addItemWithTitle:NSLocalizedString(@"IDS_REPORT_SPAM", nil) action:@selector(onReportClicked:) keyEquivalent:@""];
            
            bool isStreaming = (g_videoStreamers.find(uid) != g_videoStreamers.end() && (GetTickCountMs() - g_videoStreamers[uid] < 5000));
            if (isStreaming) {
                NSMenuItem *watchItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"IDS_WATCH_VIDEO", nil) action:@selector(onWatchVideoClicked:) keyEquivalent:@""];
                watchItem.representedObject = [NSString stringWithUTF8String:uid.c_str()];
                [menu addItem:watchItem];
            }
        }
    }
}

- (void)onWatchVideoClicked:(id)sender {
    NSString *nsUid = ((NSMenuItem*)sender).representedObject;
    std::string uid = [nsUid UTF8String];
    
    NSString *dispName = nsUid;
    for (const auto& u : g_userList) {
        if (u.userId == uid) {
            NSString *rawName = [NSString stringWithUTF8String:u.userName.c_str()];
            NSRange hashRange = [rawName rangeOfString:@"#"];
            dispName = (hashRange.location != NSNotFound) ? [rawName substringToIndex:hashRange.location] : rawName;
            break;
        }
    }
    
    if (g_videoWindows.find(uid) == g_videoWindows.end()) {
        VideoWindowController *win = [[VideoWindowController alloc] initWithUserId:nsUid userName:dispName];
        g_videoWindows[uid] = win;
        [win showWindow:self];
        win.window.title = dispName;
    } else {
        [g_videoWindows[uid] showWindow:self];
        [g_videoWindows[uid].window makeKeyAndOrderFront:self];
        g_videoWindows[uid].window.title = dispName;
    }
}

- (void)onBlockClicked:(id)sender {
    NSInteger row = self.userListView.clickedRow;
    if (row >= 0 && row < g_userList.size()) {
        std::string uid = g_userList[row].userId;
        if (g_ignoredUsers.find(uid) == g_ignoredUsers.end()) {
            g_ignoredUsers.insert(uid);
            [self appendLogMessage:[NSString stringWithFormat:@"Blocked user: %s", g_userList[row].userName.c_str()]];
        } else {
            g_ignoredUsers.erase(uid);
            [self appendLogMessage:[NSString stringWithFormat:@"Unblocked user: %s", g_userList[row].userName.c_str()]];
        }
        [self updateUserListUI];
    }
}

- (void)onReportClicked:(id)sender {
    NSInteger row = self.userListView.clickedRow;
    if (row >= 0 && row < g_userList.size()) {
        std::string uid = g_userList[row].userId;
        if (chatNetwork) {
            NSString *jsonStr = [NSString stringWithFormat:@"{\\"type\\":\\"report\\", \\"target\\":\\"%s\\"}\\n", uid.c_str()];
            chatNetwork->SendMessage([jsonStr UTF8String]);
            [self appendLogMessage:[NSString stringWithFormat:@"Reported user: %s", g_userList[row].userName.c_str()]];
        }
    }
}

- (BOOL)textView:(NSTextView *)textView doCommandBySelector:(SEL)commandSelector {
    if (commandSelector == @selector(insertNewline:)) {
        NSEvent *event = [NSApp currentEvent];
        if (event.modifierFlags & NSEventModifierFlagControl) {
            [textView insertText:@"\\n" replacementRange:textView.selectedRange];
            return YES;
        } else {
            [self onSendClicked:nil];
            return YES;
        }
    }
    return NO;
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}
""")

with open("main_reconstructed.mm", "w") as f:
    for line in new_lines:
        if isinstance(line, str):
            f.write(line)
        else:
            f.write(line.decode('utf-8'))
