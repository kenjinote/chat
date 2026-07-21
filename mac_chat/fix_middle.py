import re

with open("main.mm", "r") as f:
    lines = f.readlines()

new_lines = []
for i in range(552):
    new_lines.append(lines[i])

new_lines.append("""@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    chatNetwork = std::make_unique<ChatNetwork>();
    audioEngine = std::make_unique<MacAudioEngine>();
    videoEngine = std::make_unique<MacVideoEngine>();
    
    audioEngine->Initialize();
    audioEngine->StartPlayback();
    videoEngine->InitializeDecoder();
    
    audioEngine->SetAudioCallback([self](const uint8_t* data, size_t size) {
        self->chatNetwork->SendAudioData(data, size);
    });

    __weak typeof(self) weakSelf = self;
    audioEngine->SetVolumeCallback([weakSelf](int volume) {
        dispatch_async(dispatch_get_main_queue(), ^{
            AppDelegate *strongSelf = weakSelf;
            if (strongSelf) {
                strongSelf.micVolumeIndicator.doubleValue = volume;
            }
        });
    });

    videoEngine->SetVideoCallback([self](const uint8_t* data, size_t size, bool isH264) {
        self->chatNetwork->SendVideoData(data, size, isH264);
        
        NSData *videoData = [NSData dataWithBytes:data length:size];
        dispatch_async(dispatch_get_main_queue(), ^{
            auto it = g_videoWindows.find(g_userId);
            if (it != g_videoWindows.end()) {
                [it->second processVideoData:(const uint8_t*)videoData.bytes size:videoData.length isH264:isH264];
            }
        });
    });

    chatNetwork->SetAudioReceiveCallback([self](const std::u16string& userId, const uint8_t* data, size_t size) {
        NSString *nsId = [NSString stringWithCharacters:(const unichar*)userId.c_str() length:userId.length()];
        if (g_ignoredUsers.find([nsId UTF8String]) != g_ignoredUsers.end()) return;
        self->audioEngine->ProcessIncomingAudio(data, size);
    });

    chatNetwork->SetVideoReceiveCallback([self](const std::u16string& userId, const uint8_t* data, size_t size, bool isH264) {
        NSString *nsId = [NSString stringWithCharacters:(const unichar*)userId.c_str() length:userId.length()];
        std::string uid = [nsId UTF8String];
        if (g_ignoredUsers.find(uid) != g_ignoredUsers.end()) return;
        
        NSData *videoData = [NSData dataWithBytes:data length:size];
        dispatch_async(dispatch_get_main_queue(), ^{
            g_videoStreamers[uid] = GetTickCountMs();
            auto it = g_videoWindows.find(uid);
            if (it != g_videoWindows.end()) {
                [it->second processVideoData:(const uint8_t*)videoData.bytes size:videoData.length isH264:isH264];
            }
        });
    });

    chatNetwork->SetMessageCallback([weakSelf](const std::string& data) {
        std::string dataCopy = data;
        CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopCommonModes, ^{
            AppDelegate *strongSelf = weakSelf;
            if (!strongSelf) return;
            std::stringstream ss(dataCopy);
            std::string line;
            while (std::getline(ss, line)) {
                while (!line.empty() && (line.back() == '\\r' || line.back() == '\\n' || line.back() == ' ' || line.back() == '\\t')) {
                    line.pop_back();
                }
                if (line.empty()) continue;
                NSData *jsonData = [NSData dataWithBytes:line.data() length:line.length()];
                NSError *error = nil;
                NSDictionary *dict = [NSJSONSerialization JSONObjectWithData:jsonData options:0 error:&error];
                if (!error && dict) {
                    NSString *type = dict[@"type"];
                    if ([type isEqualToString:@"users"]) {
                        g_userList.clear();
                        NSArray *users = dict[@"users"];
                        std::set<std::string> currentUsers;
                        if (users) {
                            for (NSDictionary *u in users) {
                                UserInfo info;
                                info.userId = [u[@"userId"] UTF8String];
                                info.userName = [u[@"userName"] UTF8String];
                                info.color = [u[@"nclr"] intValue];
                                g_userList.push_back(info);
                                currentUsers.insert(info.userId);
                            }
                        }
                        [strongSelf updateUserListUI];
                        
                        NSMutableArray *windowsToClose = [NSMutableArray array];
                        for (auto const& [uid, controller] : g_videoWindows) {
                            if (currentUsers.find(uid) == currentUsers.end()) {
                                [windowsToClose addObject:controller];
                            }
                        }
                        for (VideoWindowController *controller in windowsToClose) {
                            [controller close];
                        }
                    } else if ([type isEqualToString:@"join"]) {
                        UserInfo info;
                        info.userId = [dict[@"userId"] UTF8String];
                        info.userName = [dict[@"userName"] UTF8String];
                        info.color = [dict[@"nclr"] intValue];
                        
                        bool found = false;
                        for (const auto& u : g_userList) {
                            if (u.userId == info.userId) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) g_userList.push_back(info);
                        [strongSelf updateUserListUI];
                    } else if ([type isEqualToString:@"leave"]) {
                        std::string uid = [dict[@"userId"] UTF8String];
                        g_userList.erase(std::remove_if(g_userList.begin(), g_userList.end(), [&](const UserInfo& u) { return u.userId == uid; }), g_userList.end());
                        [strongSelf updateUserListUI];
                    } else if ([type isEqualToString:@"rename"]) {
                        std::string uid = [dict[@"target"] UTF8String];
                        std::string newName = [dict[@"name"] UTF8String];
                        for (auto& u : g_userList) {
                            if (u.userId == uid) {
                                u.userName = newName;
                                break;
                            }
                        }
                        [strongSelf updateUserListUI];
                    } else if ([type isEqualToString:@"join_resp"] || [type isEqualToString:@"welcome"]) {
                        NSString *uid = dict[@"userId"];
                        if (uid) g_userId = [uid UTF8String];
                        if (strongSelf->chatNetwork) strongSelf->chatNetwork->SendMessage("{\\"type\\":\\"get_rooms\\"}\\n");
                    } else if ([type isEqualToString:@"redirect"]) {
                        NSString *ip = dict[@"ip"];
                        int port = [dict[@"port"] intValue];
                        if (ip) {
                            std::string newIp = [ip UTF8String];
                            if (strongSelf->chatNetwork) {
                                strongSelf->chatNetwork->Connect(newIp, port, 4001);
                                NSString *room = strongSelf.currentRoomName ? strongSelf.currentRoomName : NSLocalizedString(@"IDS_LOBBY", nil);
                                NSString *rejoinMsg = [NSString stringWithFormat:@"{\\"type\\":\\"join\\", \\"secretId\\":\\"%s\\", \\"userName\\":\\"%s\\", \\"roomName\\":\\"%@\\", \\"pub\\":true, \\"nclr\\":0}\\n",
                                                       g_secretId.c_str(), g_localUserName.c_str(), room];
                                strongSelf->chatNetwork->SendMessage([rejoinMsg UTF8String]);
                            }
                        }
                    } else if ([type isEqualToString:@"rooms"]) {
                        NSArray *rooms = dict[@"rooms"];
                        if (rooms) {
                            extern NSArray *g_lastPublicRooms; // Assuming this is defined
                            g_lastPublicRooms = rooms;
                            // Update MoveRoomWindow if open
                        }
                    } else if ([type isEqualToString:@"chat"]) {
                        NSString *user = dict[@"user"];
                        std::string senderName = user ? [user UTF8String] : "";
                        bool isIgnored = false;
                        for (const auto& u : g_userList) {
                            if (u.userName == senderName && g_ignoredUsers.find(u.userId) != g_ignoredUsers.end()) {
                                isIgnored = true;
                                break;
                            }
                        }
                        if (isIgnored) continue;
                        NSString *msg = dict[@"msg"];
                        BOOL b = [dict[@"b"] boolValue];
                        BOOL i = [dict[@"i"] boolValue];
                        BOOL u = [dict[@"u"] boolValue];
                        int sz = [dict[@"sz"] intValue];
                        int clr = [dict[@"clr"] intValue];
                        CGFloat red = (clr & 0xFF) / 255.0;
                        CGFloat green = ((clr >> 8) & 0xFF) / 255.0;
                        CGFloat blue = ((clr >> 16) & 0xFF) / 255.0;
                        NSColor *textColor = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
                        
                        if (sz <= 0) sz = 12;
                        NSFontTraitMask traits = 0;
                        if (b) traits |= NSBoldFontMask;
                        if (i) traits |= NSItalicFontMask;
                        NSFont *font = [[NSFontManager sharedFontManager] fontWithFamily:@"Helvetica" traits:traits weight:5 size:sz];
                        if (!font) font = [NSFont systemFontOfSize:sz];
                        NSMutableDictionary *attrs = [NSMutableDictionary dictionary];
                        attrs[NSFontAttributeName] = font;
                        attrs[NSForegroundColorAttributeName] = textColor;
                        if (u) attrs[NSUnderlineStyleAttributeName] = @(NSUnderlineStyleSingle);
                        NSString *rawName = dict[@"userName"] ? dict[@"userName"] : (dict[@"user"] ? dict[@"user"] : @"Unknown");
                        NSRange hashRange = [rawName rangeOfString:@"#"];
                        NSString *dispName = (hashRange.location != NSNotFound) ? [rawName substringToIndex:hashRange.location] : rawName;
                        int nclr = [dict[@"nclr"] intValue];
                        NSColor *nameColor = [NSColor blackColor];
                        if (nclr != -1 && nclr != 0xFFFFFFFF) {
                            CGFloat nred = (nclr & 0xFF) / 255.0;
                            CGFloat ngreen = ((nclr >> 8) & 0xFF) / 255.0;
                            CGFloat nblue = ((nclr >> 16) & 0xFF) / 255.0;
                            nameColor = [NSColor colorWithCalibratedRed:nred green:ngreen blue:nblue alpha:1.0];
                        }
                        NSMutableAttributedString *finalStr = [[NSMutableAttributedString alloc] init];
                        NSDictionary *nameAttrs = @{ NSFontAttributeName: [NSFont systemFontOfSize:12], NSForegroundColorAttributeName: nameColor };
                        [finalStr appendAttributedString:[[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@"%@: ", dispName] attributes:nameAttrs]];
                        [finalStr appendAttributedString:[[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@"%@\\n", msg ? msg : @""] attributes:attrs]];
                        [[strongSelf.logTextView textStorage] appendAttributedString:finalStr];
                        [strongSelf.logTextView scrollRangeToVisible:NSMakeRange(strongSelf.logTextView.string.length, 0)];
                    }
                }
            }
        });
        CFRunLoopWakeUp(CFRunLoopGetMain());
    });

    chatNetwork->SetConnectionCallback([weakSelf](bool connected) {
        dispatch_async(dispatch_get_main_queue(), ^{
            AppDelegate *strongSelf = weakSelf;
            if (strongSelf) {
                [strongSelf appendSystemLogMessage:connected ? @"Connected to server." : @"Disconnected from server."];
            }
        });
    });

    NSRect frame = NSMakeRect(0, 0, 800, 600);
    NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
    self.window = [[SnappableWindow alloc] initWithContentRect:frame styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
    [self.window setTitle:@"MacChat"];
    [self.window center];
    self.window.delegate = self;
    
    self.logScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    self.logScroll.hasVerticalScroller = YES;
    self.logTextView = [[NSTextView alloc] initWithFrame:self.logScroll.bounds];
    self.logTextView.editable = NO;
    self.logTextView.richText = YES;
    self.logScroll.documentView = self.logTextView;
    [self.window.contentView addSubview:self.logScroll];
    
    self.userListHeader = [[NSTextField alloc] initWithFrame:NSZeroRect];
    self.userListHeader.editable = NO;
    self.userListHeader.bordered = NO;
    self.userListHeader.drawsBackground = NO;
    self.userListHeader.alignment = NSTextAlignmentCenter;
    [self.window.contentView addSubview:self.userListHeader];
    
    self.userScroll = [[NSScrollView alloc] initWithFrame:NSZeroRect];
    self.userScroll.hasVerticalScroller = YES;
    self.userListView = [[NSTableView alloc] initWithFrame:self.userScroll.bounds];
    NSTableColumn *col = [[NSTableColumn alloc] initWithIdentifier:@"Users"];
    col.width = 150;
    [self.userListView addTableColumn:col];
    self.userListView.headerView = nil;
    self.userListView.dataSource = self;
    self.userListView.delegate = self;
    self.userListView.menu = [[NSMenu alloc] initWithTitle:@"Context"];
    self.userListView.menu.delegate = self;
    self.userScroll.documentView = self.userListView;
    [self.window.contentView addSubview:self.userScroll];
    
    self.btnB = [[NSButton alloc] initWithFrame:NSZeroRect];
    [self.btnB setButtonType:NSButtonTypePushOnPushOff];
    self.btnB.title = @"B";
    self.btnB.target = self;
    self.btnB.action = @selector(onFormatChanged:);
    [self.window.contentView addSubview:self.btnB];
    
    self.btnI = [[NSButton alloc] initWithFrame:NSZeroRect];
    [self.btnI setButtonType:NSButtonTypePushOnPushOff];
    self.btnI.title = @"I";
    self.btnI.target = self;
    self.btnI.action = @selector(onFormatChanged:);
    [self.window.contentView addSubview:self.btnI];
    
    self.btnU = [[NSButton alloc] initWithFrame:NSZeroRect];
    [self.btnU setButtonType:NSButtonTypePushOnPushOff];
    self.btnU.title = @"U";
    self.btnU.target = self;
    self.btnU.action = @selector(onFormatChanged:);
    [self.window.contentView addSubview:self.btnU];
    
    self.sizeCombo = [[NSComboBox alloc] initWithFrame:NSZeroRect];
    [self.sizeCombo addItemsWithObjectValues:@[@"6", @"8", @"9", @"10", @"11", @"12", @"14", @"16", @"18", @"20", @"22", @"24", @"26", @"28", @"32"]];
    [self.sizeCombo selectItemAtIndex:5];
    self.sizeCombo.target = self;
    self.sizeCombo.action = @selector(onFormatChanged:);
    self.sizeCombo.delegate = self;
    [self.window.contentView addSubview:self.sizeCombo];
    
    self.colorWell = [[NSColorWell alloc] initWithFrame:NSZeroRect];
    self.colorWell.target = self;
    self.colorWell.action = @selector(onFormatChanged:);
    [self.window.contentView addSubview:self.colorWell];
    
    self.currentRoomName = NSLocalizedString(@"IDS_LOBBY", nil);
    [self updateWindowTitle];
    self.moveRoomBtn = [[NSButton alloc] initWithFrame:NSZeroRect];
    self.moveRoomBtn.title = NSLocalizedString(@"IDS_MOVE_ROOM", nil);
    self.moveRoomBtn.target = self;
    self.moveRoomBtn.action = @selector(onMoveRoomClicked:);
    [self.window.contentView addSubview:self.moveRoomBtn];
    
    self.voiceButton = [[NSButton alloc] initWithFrame:NSZeroRect];
    self.voiceButton.title = [NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_TALK", nil)];
    self.voiceButton.target = self;
    self.voiceButton.action = @selector(onVoiceClicked:);
    [self.window.contentView addSubview:self.voiceButton];
    
    self.videoButton = [[NSButton alloc] initWithFrame:NSZeroRect];
    self.videoButton.title = [NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_VIDEO_STREAM", nil)];
    self.videoButton.target = self;
    self.videoButton.action = @selector(onVideoClicked:);
    [self.window.contentView addSubview:self.videoButton];
    
    self.micVolumeIndicator = [[NSLevelIndicator alloc] initWithFrame:NSZeroRect];
    self.micVolumeIndicator.levelIndicatorStyle = NSLevelIndicatorStyleContinuousCapacity;
    [self.window.contentView addSubview:self.micVolumeIndicator];
    
    self.volLabel = [[NSTextField alloc] initWithFrame:NSZeroRect];
    self.volLabel.stringValue = NSLocalizedString(@"IDS_VOL", nil);
    self.volLabel.editable = NO;
    self.volLabel.bordered = NO;
    self.volLabel.drawsBackground = NO;
    [self.window.contentView addSubview:self.volLabel];
    
    self.volumeSlider = [[NSSlider alloc] initWithFrame:NSZeroRect];
    self.volumeSlider.doubleValue = 1.0;
    self.volumeSlider.target = self;
    self.volumeSlider.action = @selector(onVolumeSliderChanged:);
    [self.window.contentView addSubview:self.volumeSlider];
    
    self.maintenanceView = [[NSTextView alloc] initWithFrame:NSZeroRect];
    self.maintenanceView.hidden = YES;
    [self.window.contentView addSubview:self.maintenanceView];
    
    self.inputScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    self.inputTextView = [[NSTextView alloc] initWithFrame:self.inputScroll.bounds];
    self.inputTextView.delegate = self;
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
""")

for i in range(907, len(lines)):
    new_lines.append(lines[i])

with open("main_reconstructed2.mm", "w") as f:
    for line in new_lines:
        if isinstance(line, str):
            f.write(line)
        else:
            f.write(line.decode('utf-8'))
