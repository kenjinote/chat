                    if (layer.intValue == 0 && owner.length > 0 && title.length > 0) {
                        if ([owner isEqualToString:@"MacChat"] || [owner isEqualToString:@"Terminal"]) continue;
                        
                        NSString *menuTitle = [NSString stringWithFormat:@"[%@] %@", owner, title];
static std::string g_userId = "";
                        NSAttributedString *attrStr = [[NSAttributedString alloc] initWithHTML:htmlData documentAttributes:nil];
@class VideoWindowController;
static std::map<std::string, VideoWindowController*> g_videoWindows;

    extern ChatNetwork *g_chatNetworkInstance;
    // We will trigger room list refresh
            std::cout << "[DEBUG] updatePublicRooms adding room: " << [name UTF8String] << " count: " << count << std::endl;
        }
    }
    std::cout << "[DEBUG] updatePublicRooms total rooms updated: " << self.allRooms.count << std::endl;
    std::cout << "[DEBUG] filterRooms filtered count: " << self.filteredRooms.count << std::endl;


    // Initialize engines
    // Initialize audio and set callback
        dispatch_async(dispatch_get_main_queue(), ^{
            VideoWindowController *controller = nullptr;
            if (it == g_videoWindows.end()) {
                NSString *uName = nsId;
                for (const auto& u : g_userList) {
                    if (u.userId == uid) {
                        uName = [NSString stringWithUTF8String:u.userName.c_str()];
                        break;
                    }
                }
                controller = [[VideoWindowController alloc] initWithUserId:nsId userName:uName];
                g_videoWindows[uid] = controller;
                [controller showWindow:nil];
            } else {
                controller = it->second;
            }
            
            [controller processVideoData:data size:size isH264:isH264];
                if (error) {
                    std::cout << "[DEBUG] JSON parse error: " << [[error localizedDescription] UTF8String] << " on line: " << line << std::endl;
                }
                    std::cout << "[DEBUG] SetMessageCallback received type: " << (type ? [type UTF8String] : "null") << std::endl;
                        
                        NSString *rawName = [NSString stringWithUTF8String:g_localUserName.c_str()];
                        NSRange hashRange = [rawName rangeOfString:@"#"];
                        NSString *dispName = (hashRange.location != NSNotFound) ? [rawName substringToIndex:hashRange.location] : rawName;
                        NSString *roomName = strongSelf.currentRoomName ? strongSelf.currentRoomName : NSLocalizedString(@"IDS_LOBBY", nil);
                        [strongSelf appendSystemLogMessage:[NSString stringWithFormat:NSLocalizedString(@"IDS_ROOM_ENTER_FMT", nil), dispName, roomName]];
                        
                        
                        NSString *rawName = dict[@"userName"];
                        NSRange hashRange = [rawName rangeOfString:@"#"];
                        NSString *dispName = (hashRange.location != NSNotFound) ? [rawName substringToIndex:hashRange.location] : rawName;
                        NSString *roomName = strongSelf.currentRoomName ? strongSelf.currentRoomName : NSLocalizedString(@"IDS_LOBBY", nil);
                        [strongSelf appendSystemLogMessage:[NSString stringWithFormat:NSLocalizedString(@"IDS_ROOM_ENTER_FMT", nil), dispName, roomName]];
                        
                        
                        NSString *dispName = @"Unknown";
                        for (const auto& u : g_userList) {
                            if (u.userId == uid) {
                                NSString *rawName = [NSString stringWithUTF8String:u.userName.c_str()];
                                NSRange hashRange = [rawName rangeOfString:@"#"];
                                dispName = (hashRange.location != NSNotFound) ? [rawName substringToIndex:hashRange.location] : rawName;
                                break;
                            }
                        }
                        
                        
                        NSString *roomName = strongSelf.currentRoomName ? strongSelf.currentRoomName : NSLocalizedString(@"IDS_LOBBY", nil);
                        [strongSelf appendSystemLogMessage:[NSString stringWithFormat:NSLocalizedString(@"IDS_ROOM_LEAVE_FMT", nil), dispName, roomName]];
                        
                        if (uid) {
                            g_userId = [uid UTF8String];
                        }
                        if (strongSelf->chatNetwork) {
                            strongSelf->chatNetwork->SendMessage("{\"type\":\"get_rooms\"}\n");
                        }
                            std::cout << "Redirecting to server: " << newIp << ":" << port << std::endl;
                                
                                [strongSelf appendSystemLogMessage:[NSString stringWithFormat:NSLocalizedString(@"IDS_MOVED_TO", nil), room]];
                                
                                strongSelf->chatNetwork->SendMessage("{\"type\":\"get_rooms\"}\n");
                            }
                        }
                    } else if ([type isEqualToString:@"room_list"] || [type isEqualToString:@"rooms"]) {
                            if (g_activeMoveRoomDlg) {
                                [g_activeMoveRoomDlg updatePublicRooms:rooms];
                            }
                        

                        
                        
                        
                        
                        
                        if (u) {
                            attrs[NSUnderlineStyleAttributeName] = @(NSUnderlineStyleSingle);
                        }
                        
                        
                        
                        
        if (strongSelf && strongSelf->chatNetwork) {
            std::cout << "[DEBUG] RequestRoomListNotification: Sending get_rooms" << std::endl;
            strongSelf->chatNetwork->SendMessage("{\"type\":\"get_rooms\"}\n");
        }
    }];
    
    // Create the main window
    self.window = [[SnappableWindow alloc] initWithContentRect:frame
                                                     styleMask:styleMask
                                                       backing:NSBackingStoreBuffered
                                                         defer:NO];
    self.window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
    // 1. Chat Log View (NSTextView inside NSScrollView)
    
    self.logTextView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    // Move Room Button
    // User List Header & View
    self.userListHeader.stringValue = NSLocalizedString(@"IDS_USER_LIST_0", nil);
    self.userListHeader.font = [NSFont boldSystemFontOfSize:12];

    
    
    NSMenu *contextMenu = [[NSMenu alloc] initWithTitle:@"Context"];
    contextMenu.delegate = self;
    self.userListView.menu = contextMenu;
    
    // Formatting Tools
    [self.sizeCombo selectItemAtIndex:5]; // Default to 12
    self.colorWell.color = [NSColor blackColor];

    // Voice & Media Controls
    self.voiceButton.bezelStyle = NSBezelStyleRounded;
    self.micVolumeIndicator.minValue = 0;
    self.micVolumeIndicator.maxValue = 100;
    self.micVolumeIndicator.doubleValue = 0;
    self.micVolumeIndicator.warningValue = 80;
    self.micVolumeIndicator.criticalValue = 95;

    self.volumeSlider.minValue = 0.0;
    self.volumeSlider.maxValue = 1.0;
    self.videoButton.bezelStyle = NSBezelStyleRounded;

    // Maintenance View
    self.maintenanceView.editable = NO;
    self.maintenanceView.selectable = YES;
    self.maintenanceView.drawsBackground = NO;
    self.maintenanceView.linkTextAttributes = @{
        NSForegroundColorAttributeName: [NSColor systemBlueColor],
        NSUnderlineStyleAttributeName: @(NSUnderlineStyleSingle),
        NSCursorAttributeName: [NSCursor pointingHandCursor]
    };

    // Multiline Input TextView
    self.inputScroll.hasVerticalScroller = YES;
    self.inputTextView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    self.inputTextView.richText = NO;
    // Send Button
    self.sendButton.bezelStyle = NSBezelStyleRegularSquare;
    // Connect to signaling/chat server
    std::string serverHost = "chat-lb-2026.japaneast.cloudapp.azure.com";
    int tcpPort = 4000;
    int udpPort = 4001;

    if (g_secretId.empty()) {
        g_secretId = "mac_" + std::to_string(arc4random());
    }
    if (g_localUserName.empty()) {
        g_localUserName = "MacUser#" + std::to_string(1000 + arc4random() % 9000);
    }

    if (chatNetwork->Connect(serverHost, tcpPort, udpPort)) {
        std::cout << "Connected to chat server: " << serverHost << std::endl;
    } else {
        std::cout << "Failed to connect to primary server " << serverHost << ", trying fallback 127.0.0.1..." << std::endl;
        if (chatNetwork->Connect("127.0.0.1", tcpPort, udpPort)) {
            NSString *joinMsg = [NSString stringWithFormat:@"{\"type\":\"join\", \"secretId\":\"%s\", \"userName\":\"%s\", \"roomName\":\"Lobby\", \"pub\":true, \"nclr\":0}\n",
                                 g_secretId.c_str(), g_localUserName.c_str()];
            chatNetwork->SendMessage([joinMsg UTF8String]);
            chatNetwork->SendMessage("{\"type\":\"get_rooms\"}\n");
        }
    }

    // Auto-reconnect timer
    [NSTimer scheduledTimerWithTimeInterval:3.0 repeats:YES block:^(NSTimer * _Nonnull timer) {
        AppDelegate *strongSelf = weakSelf;
        if (strongSelf && strongSelf->chatNetwork && !strongSelf->chatNetwork->IsConnected()) {
            if (strongSelf->chatNetwork->Connect(serverHost, tcpPort, udpPort) || strongSelf->chatNetwork->Connect("127.0.0.1", tcpPort, udpPort)) {
                NSString *room = strongSelf.currentRoomName ? strongSelf.currentRoomName : NSLocalizedString(@"IDS_LOBBY", nil);
                NSString *rejoinMsg = [NSString stringWithFormat:@"{\"type\":\"join\", \"secretId\":\"%s\", \"userName\":\"%s\", \"roomName\":\"%@\", \"pub\":true, \"nclr\":0}\n",
                                       g_secretId.c_str(), g_localUserName.c_str(), room];
                strongSelf->chatNetwork->SendMessage([rejoinMsg UTF8String]);
                strongSelf->chatNetwork->SendMessage("{\"type\":\"get_rooms\"}\n");
            }
        }
    }];

    [self fetchMaintenanceMessage];
    [NSTimer scheduledTimerWithTimeInterval:3600 repeats:YES block:^(NSTimer * _Nonnull timer) {
        [self fetchMaintenanceMessage];
    }];
    std::cout << "Application launched!" << std::endl;
    NSString *rawName = [NSString stringWithUTF8String:g_localUserName.c_str()];
    NSRange hashRange = [rawName rangeOfString:@"#"];
    NSString *dispName = (hashRange.location != NSNotFound) ? [rawName substringToIndex:hashRange.location] : rawName;
    NSString *joinedMsg = [NSString stringWithFormat:@"%@ (%@)", NSLocalizedString(@"IDS_JOINED_NETWORK", nil), dispName];
    [self appendSystemLogMessage:joinedMsg];
    NSURLRequest *request = [NSURLRequest requestWithURL:url cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:10.0];
    NSURLSessionDataTask *task = [[NSURLSession sharedSession] dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
            NSError *jsonError = nil;
            NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&jsonError];
            if (!jsonError && [json isKindOfClass:[NSDictionary class]]) {
                NSString *lang = [[[NSLocale currentLocale] languageCode] isEqualToString:@"ja"] ? @"ja" : @"en";
                NSString *msg = json[lang];
                if (!msg || msg.length == 0) msg = json[@"en"];
                if (!msg || msg.length == 0) msg = json[@"ja"];
                
                dispatch_async(dispatch_get_main_queue(), ^{
                    if (msg && msg.length > 0) {
                        NSData *htmlData = [msg dataUsingEncoding:NSUTF8StringEncoding];
                        NSDictionary *options = @{
                            NSDocumentTypeDocumentAttribute: NSHTMLTextDocumentType,
                            NSCharacterEncodingDocumentOption: @(NSUTF8StringEncoding)
                        };
                        NSAttributedString *attrStr = [[NSAttributedString alloc] initWithData:htmlData options:options documentAttributes:nil error:nil];
                        if (attrStr) {
                            NSMutableAttributedString *mutAttrStr = [attrStr mutableCopy];
                            [mutAttrStr addAttribute:NSFontAttributeName value:[NSFont boldSystemFontOfSize:12] range:NSMakeRange(0, mutAttrStr.length)];
                            [self.maintenanceView.textStorage setAttributedString:mutAttrStr];
                            self.maintenanceView.hidden = NO;
                        }
                    } else {
                        self.maintenanceView.hidden = YES;
                    }
    
    CGFloat width = bounds.size.width;
    CGFloat height = bounds.size.height;
    
    CGFloat margin = 5.0;
    CGFloat inputHeight = 40.0;
    CGFloat btnWidth = 60.0;
    CGFloat listWidth = 150.0;
    CGFloat toolHeight = 24.0;
    CGFloat roomBtnHeight = 30.0;
    CGFloat headerHeight = 20.0;
    
    CGFloat maintenanceHeight = (!self.maintenanceView || self.maintenanceView.hidden) ? 0.0 : 16.0;
    CGFloat maintenanceMargin = (maintenanceHeight > 0) ? 2.0 : 0.0;
    
    CGFloat topHeight = height - inputHeight - toolHeight - maintenanceHeight - maintenanceMargin - margin * 4;
    if (topHeight < 50) topHeight = 50;
    
    CGFloat logWidth = width - listWidth - margin * 3;
    if (logWidth < 100) logWidth = 100;
    
    CGFloat topPanelBottomY = margin * 3 + inputHeight + toolHeight + maintenanceHeight + maintenanceMargin;
    
    // 1. Chat Log
    self.logScroll.frame = NSMakeRect(margin, topPanelBottomY, logWidth, topHeight);
    
    // 2. Right Panel (User Header, User List, Move Room Button)
    CGFloat rightPanelX = margin * 2 + logWidth;
    
    self.userListHeader.frame = NSMakeRect(rightPanelX, topPanelBottomY + topHeight - headerHeight, listWidth, headerHeight);
    
    self.moveRoomBtn.frame = NSMakeRect(rightPanelX, topPanelBottomY, listWidth, roomBtnHeight);
    
    CGFloat userListY = topPanelBottomY + roomBtnHeight + margin;
    CGFloat userListHeight = topHeight - roomBtnHeight - headerHeight - margin;
    if (userListHeight < 30) userListHeight = 30;
    self.userScroll.frame = NSMakeRect(rightPanelX, userListY, listWidth, userListHeight);
    
    // 3. Toolbar Row (Right under Chat Log)
    CGFloat toolY = margin * 2 + inputHeight + maintenanceHeight + maintenanceMargin;
    CGFloat currentX = margin;
    CGFloat gap = 4.0;
    
    self.btnB.frame = NSMakeRect(currentX, toolY, 24, toolHeight);
    currentX += 24 + gap;
    
    self.btnI.frame = NSMakeRect(currentX, toolY, 24, toolHeight);
    currentX += 24 + gap;
    
    self.btnU.frame = NSMakeRect(currentX, toolY, 24, toolHeight);
    currentX += 24 + gap;
    
    self.sizeCombo.frame = NSMakeRect(currentX, toolY, 48, toolHeight);
    currentX += 48 + gap;
    
    self.colorWell.frame = NSMakeRect(currentX, toolY, 32, toolHeight);
    currentX += 32 + gap;
    
    self.voiceButton.frame = NSMakeRect(currentX, toolY, 70, toolHeight);
    currentX += 70 + gap;
    
    self.micVolumeIndicator.frame = NSMakeRect(currentX, toolY + (toolHeight - 14) / 2, 70, 14);
    currentX += 70 + gap;
    
    self.volLabel.frame = NSMakeRect(currentX, toolY + (toolHeight - 18) / 2, 55, 18);
    currentX += 55 + gap;
    
    self.volumeSlider.frame = NSMakeRect(currentX, toolY + (toolHeight - 18) / 2, 90, 18);
    currentX += 90 + gap;
    
    self.videoButton.frame = NSMakeRect(currentX, toolY, 75, toolHeight);
    
    // 4. Bottom Input & Send Button Area
    CGFloat bottomY = margin;
    
    if (maintenanceHeight > 0) {
        self.maintenanceView.frame = NSMakeRect(margin, bottomY, width - margin * 2, maintenanceHeight);
        bottomY += maintenanceHeight + maintenanceMargin;
    }
    
    CGFloat inputWidth = width - btnWidth - margin * 3;
    if (inputWidth < 100) inputWidth = 100;
    
    self.inputScroll.frame = NSMakeRect(margin, bottomY, inputWidth, inputHeight);
    self.sendButton.frame = NSMakeRect(margin * 2 + inputWidth, bottomY, btnWidth, inputHeight);
}

- (void)windowDidResize:(NSNotification *)notification {
    [self layoutControls];
    NSString *room = self.currentRoomName ? self.currentRoomName : NSLocalizedString(@"IDS_LOBBY", nil);
    NSString *title = [NSString stringWithFormat:[NSString stringWithFormat:@"%@ - [%%@]", NSLocalizedString(@"IDS_WINDOW_TITLE", nil)], room];
    bool isBold = self.btnB.state == NSControlStateValueOn;
    bool isItalic = self.btnI.state == NSControlStateValueOn;
    bool isUnderline = self.btnU.state == NSControlStateValueOn;
    
    int fontSize = 12;
    NSInteger idx = self.sizeCombo.indexOfSelectedItem;
    if (idx >= 0 && idx < self.sizeCombo.numberOfItems) {
        fontSize = [[self.sizeCombo itemObjectValueAtIndex:idx] intValue];
    } else {
        fontSize = self.sizeCombo.intValue;
    }
    if (fontSize <= 0) fontSize = 12; // fallback
    
    if (isBold) traits |= NSBoldFontMask;
    if (isItalic) traits |= NSItalicFontMask;
    
    NSFont *font = [[NSFontManager sharedFontManager] fontWithFamily:@"Helvetica" traits:traits weight:5 size:fontSize];
    if (!font) font = [NSFont systemFontOfSize:fontSize];
    
    NSColor *color = self.colorWell.color;
    
    NSMutableDictionary *attrs = [NSMutableDictionary dictionary];
    if (font) attrs[NSFontAttributeName] = font;
    if (color) attrs[NSForegroundColorAttributeName] = color;
    if (isUnderline) {
        attrs[NSUnderlineStyleAttributeName] = @(NSUnderlineStyleSingle);
    }
    
    [self.inputTextView.textStorage setAttributes:attrs range:NSMakeRange(0, self.inputTextView.string.length)];
}

- (void)comboBoxSelectionDidChange:(NSNotification *)notification {
    if (notification.object == self.sizeCombo) {
        // Need to run on next runloop cycle because the text field value might not be updated yet
        dispatch_async(dispatch_get_main_queue(), ^{
            [self onFormatChanged:nil];
        });
    }
    NSString *newText = [NSString stringWithFormat:@"%@\n", message];
    NSAttributedString *attrStr = [[NSAttributedString alloc] initWithString:newText];
    [[self.logTextView textStorage] appendAttributedString:attrStr];
    [self.logTextView scrollRangeToVisible:NSMakeRange(self.logTextView.string.length, 0)];
    NSString *newText = [NSString stringWithFormat:@"%@\n", message];
    NSDictionary *attrs = @{ NSFontAttributeName: [NSFont systemFontOfSize:12], NSForegroundColorAttributeName: [NSColor grayColor] };
    NSAttributedString *attrStr = [[NSAttributedString alloc] initWithString:newText attributes:attrs];
    [[self.logTextView textStorage] appendAttributedString:attrStr];
    [self.logTextView scrollRangeToVisible:NSMakeRange(self.logTextView.string.length, 0)];
    if (msg.length > 0) {
        bool isBold = self.btnB.state == NSControlStateValueOn;
        bool isItalic = self.btnI.state == NSControlStateValueOn;
        bool isUnderline = self.btnU.state == NSControlStateValueOn;
        
        int fontSize = 12;
        NSInteger idx = self.sizeCombo.indexOfSelectedItem;
        if (idx >= 0 && idx < self.sizeCombo.numberOfItems) {
            fontSize = [[self.sizeCombo itemObjectValueAtIndex:idx] intValue];
        } else {
            fontSize = self.sizeCombo.intValue;
        }
        if (fontSize <= 0) fontSize = 12; // fallback
        
        CGFloat r, g, b, a;
        [[self.colorWell.color colorUsingColorSpace:[NSColorSpace sRGBColorSpace]] getRed:&r green:&g blue:&b alpha:&a];
        int colorInt = ((int)(b * 255) << 16) | ((int)(g * 255) << 8) | (int)(r * 255);
        
        // Escape newlines and quotes for raw JSON construction
        NSString *escapedMsg = [msg stringByReplacingOccurrencesOfString:@"\\" withString:@"\\\\"];
        escapedMsg = [escapedMsg stringByReplacingOccurrencesOfString:@"\"" withString:@"\\\""];
        escapedMsg = [escapedMsg stringByReplacingOccurrencesOfString:@"\n" withString:@"\\n"];
        escapedMsg = [escapedMsg stringByReplacingOccurrencesOfString:@"\r" withString:@""];
        
        NSString *jsonStr = [NSString stringWithFormat:
            @"{\"type\":\"chat\", \"user\":\"%s\", \"msg\":\"%@\", \"b\":%s, \"i\":%s, \"u\":%s, \"sz\":%d, \"clr\":%d, \"nclr\":0}\n",
            g_localUserName.c_str(),
            escapedMsg,
            isBold ? "true" : "false",
            isItalic ? "true" : "false",
            isUnderline ? "true" : "false",
            fontSize,
            colorInt
        ];
        
        if (chatNetwork) {
            chatNetwork->SendMessage([jsonStr UTF8String]);
        }
        
        // Local echo to display with formatting
        NSFontTraitMask traits = 0;
        if (isBold) traits |= NSBoldFontMask;
        if (isItalic) traits |= NSItalicFontMask;
        
        NSFont *font = [[NSFontManager sharedFontManager] fontWithFamily:@"Helvetica" traits:traits weight:5 size:fontSize];
        if (!font) font = [NSFont systemFontOfSize:fontSize];
        
        NSMutableDictionary *attrs = [NSMutableDictionary dictionary];
        if (font) attrs[NSFontAttributeName] = font;
        if (self.colorWell.color) attrs[NSForegroundColorAttributeName] = self.colorWell.color;
        if (isUnderline) attrs[NSUnderlineStyleAttributeName] = @(NSUnderlineStyleSingle);
        
        NSString *rawName = [NSString stringWithUTF8String:g_localUserName.c_str()];
        NSRange hashRange = [rawName rangeOfString:@"#"];
        NSString *dispName = (hashRange.location != NSNotFound) ? [rawName substringToIndex:hashRange.location] : rawName;
        
        NSMutableAttributedString *finalStr = [[NSMutableAttributedString alloc] init];
        NSDictionary *nameAttrs = @{ NSFontAttributeName: [NSFont systemFontOfSize:12], NSForegroundColorAttributeName: [NSColor blackColor] };
        [finalStr appendAttributedString:[[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@"%@: ", dispName] attributes:nameAttrs]];
        [finalStr appendAttributedString:[[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@"%@\n", msg] attributes:attrs]];
        
        [[self.logTextView textStorage] appendAttributedString:finalStr];
        [self.logTextView scrollRangeToVisible:NSMakeRange(self.logTextView.string.length, 0)];
        
    if (textView == self.inputTextView) {
        if (commandSelector == @selector(insertNewline:)) {
            NSEvent *event = [NSApp currentEvent];
            if (event.modifierFlags & NSEventModifierFlagControl) {
                [textView insertText:@"\n" replacementRange:textView.selectedRange];
                return YES;
            } else {
                [self onSendClicked:nil];
                return YES;
            }
        }
    }
    if ([self.voiceButton.title isEqualToString:[NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_TALK", nil)]]) {
        self.voiceButton.title = [NSString stringWithFormat:@"%@: ON", NSLocalizedString(@"IDS_TALK", nil)];
        audioEngine->StartMicrophone();
    } else {
        self.voiceButton.title = [NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_TALK", nil)];
        audioEngine->StopMicrophone();
        self.micVolumeIndicator.doubleValue = 0;
    }
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
    if (g_captureSourceType == 1) {
        g_captureSourceType = 0;
        self.videoButton.title = [NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_VIDEO_STREAM", nil)];
        videoEngine->StopCapture();
        if (g_videoWindows.find(g_userId) != g_videoWindows.end()) {
            [g_videoWindows[g_userId] close];
            g_videoWindows.erase(g_userId);
        }
    } else {
        g_captureSourceType = 1;
        self.videoButton.title = [NSString stringWithFormat:@"%@: ON", NSLocalizedString(@"IDS_VIDEO_STREAM", nil)];
        [self showLocalVideoWindowWithTitleSuffix:nil];
        videoEngine->StartCapture();
    }
}

- (void)onScreenContextClicked:(id)sender {
    if (g_captureSourceType == 2) {
        g_captureSourceType = 0;
        videoEngine->StopScreenCapture();
        if (g_videoWindows.find(g_userId) != g_videoWindows.end()) {
            [g_videoWindows[g_userId] close];
            g_videoWindows.erase(g_userId);
        }
    } else {
        g_captureSourceType = 2;
        self.videoButton.title = [NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_VIDEO_STREAM", nil)]; // Camera is disabled by engine logic
        [self showLocalVideoWindowWithTitleSuffix:@" (Screen)"];
        videoEngine->StartScreenCapture();
    }
}

- (void)onWindowShareClicked:(id)sender {
    NSMenuItem *item = (NSMenuItem*)sender;
    NSNumber *winId = item.representedObject;
    if (g_captureSourceType == 3 && g_captureWindowId == winId.unsignedIntValue) {
        g_captureSourceType = 0;
        videoEngine->StopWindowCapture();
        if (g_videoWindows.find(g_userId) != g_videoWindows.end()) {
            [g_videoWindows[g_userId] close];
            g_videoWindows.erase(g_userId);
        }
    } else {
        g_captureSourceType = 3;
        g_captureWindowId = winId.unsignedIntValue;
        self.videoButton.title = [NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_VIDEO_STREAM", nil)];
        [self showLocalVideoWindowWithTitleSuffix:@" (Window)"];
        videoEngine->StartWindowCapture(g_captureWindowId);
    }
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
                        if ([owner isEqualToString:@"MacChat"]) continue;
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
            
            [shareItem se
