import os

with open('main.mm.orig', 'r') as f:
    text = f.read()

# 1. Add globals
g_str = """static std::string g_secretId = "MacLocalSecret-12345";
#include <set>
#include <map>
#include <sys/time.h>

static std::set<std::string> g_ignoredUsers;
static std::map<std::string, VideoWindowController*> g_videoWindows;
static std::map<std::string, uint32_t> g_videoStreamers;

static uint32_t GetTickCountMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}"""
text = text.replace('static std::string g_secretId = "MacLocalSecret-12345";', g_str)

# 2. Fix VideoWindowController missing windowWillClose
v_close = """- (void)windowWillClose:(NSNotification *)notification {
    std::string uid = [self.userId UTF8String];
    auto it = g_videoWindows.find(uid);
    if (it != g_videoWindows.end()) {
        g_videoWindows.erase(it);
    }
}
@end"""
text = text.replace('@end\n\nstatic NSArray *g_lastPublicRooms = nil;', v_close + '\n\nstatic NSArray *g_lastPublicRooms = nil;')


# 3. Fix AppDelegate @interface missing micVolumeIndicator and others
app_del = """@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate, NSTableViewDataSource, NSTableViewDelegate, NSMenuDelegate, NSTextViewDelegate, NSComboBoxDelegate> {
    std::unique_ptr<ChatNetwork> chatNetwork;
    std::unique_ptr<MacAudioEngine> audioEngine;
    std::unique_ptr<MacVideoEngine> videoEngine;
}
@property (strong) SnappableWindow *window;
@property (strong) NSView *maintenanceView;
@property (strong) NSScrollView *logScroll;
@property (strong) NSTextView *logTextView;
@property (strong) NSScrollView *inputScroll;
@property (strong) NSTextView *inputTextView;
@property (strong) NSTextField *userListHeader;
@property (strong) NSScrollView *userScroll;
@property (strong) NSTableView *userListView;
@property (strong) NSButton *sendButton;
@property (strong) NSButton *voiceButton;
@property (strong) NSButton *videoButton;
@property (strong) NSButton *btnB;
@property (strong) NSButton *btnI;
@property (strong) NSButton *btnU;
@property (strong) NSComboBox *sizeCombo;
@property (strong) NSColorWell *colorWell;
@property (strong) NSLevelIndicator *micVolumeIndicator;"""
import re
text = re.sub(r'@interface AppDelegate.*?@property \(strong\) NSLevelIndicator \*micVolumeIndicator;', app_del, text, flags=re.DOTALL)


# 4. Fix AudioReceiveCallback bug and add VideoReceiveCallback
bad_audio = """    chatNetwork->SetAudioReceiveCallback([self](const std::u16string& userId, const uint8_t* data, size_t size) {
        NSString *nsId = [NSString stringWithCharacters:(const unichar*)userId.c_str() length:userId.length()];
        if (g_ignoredUsers.find([nsId UTF8String]) != g_ignoredUsers.end()) return;
        self->audioEngine->ProcessIncomingAudio(data, size);
        std::string uid = [nsId UTF8String];
        if (g_ignoredUsers.find(uid) != g_ignoredUsers.end()) return;
        
        dispatch_async(dispatch_get_main_queue(), ^{
            VideoWindowController *controller = nullptr;
            auto it = g_videoWindows.find(uid);
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
        });
    });"""

good_audio = """    chatNetwork->SetAudioReceiveCallback([self](const std::u16string& userId, const uint8_t* data, size_t size) {
        NSString *nsId = [NSString stringWithCharacters:(const unichar*)userId.c_str() length:userId.length()];
        if (g_ignoredUsers.find([nsId UTF8String]) != g_ignoredUsers.end()) return;
        self->audioEngine->ProcessIncomingAudio(data, size);
    });

    chatNetwork->SetVideoReceiveCallback([self](const std::u16string& userId, const uint8_t* data, size_t size, bool isH264) {
        std::string uid = [NSString stringWithCharacters:(const unichar*)userId.c_str() length:userId.length()].UTF8String;
        if (g_ignoredUsers.find(uid) != g_ignoredUsers.end()) return;
        g_videoStreamers[uid] = GetTickCountMs();
        
        NSData *videoData = [NSData dataWithBytes:data length:size];
        dispatch_async(dispatch_get_main_queue(), ^{
            auto it = g_videoWindows.find(uid);
            if (it != g_videoWindows.end()) {
                [it->second processVideoData:(const uint8_t*)videoData.bytes size:videoData.length isH264:isH264];
            }
        });
    });"""

if bad_audio in text:
    text = text.replace(bad_audio, good_audio)
else:
    print("WARNING: bad_audio not found")

# 5. Fix color implementation in AppDelegate
color_func = """- (int)getLocalColorInt {
    NSColor *color = [self.colorWell.color colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
    if (!color) return 0;
    int r = (int)(color.redComponent * 255.0) & 0xFF;
    int g = (int)(color.greenComponent * 255.0) & 0xFF;
    int b = (int)(color.blueComponent * 255.0) & 0xFF;
    return r | (g << 8) | (b << 16);
}

- (void)comboBoxSelectionDidChange:(NSNotification *)notification {
    [self onFormatChanged:nil];
}

- (void)onFormatChanged:(id)sender {
    NSFontTraitMask traits = 0;
    if (self.btnB.state == NSControlStateValueOn) traits |= NSBoldFontMask;
    if (self.btnI.state == NSControlStateValueOn) traits |= NSItalicFontMask;
    
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
"""
text = text.replace('- (void)comboBoxSelectionDidChange:(NSNotification *)notification {\n}', color_func)

# 6. Replace joining color
text = text.replace(', g_localColor];', ', [self getLocalColorInt]];')
text = text.replace('\"nclr\":0', '\"nclr\":%d')
text = text.replace('g_secretId.c_str(), g_localUserName.c_str()];', 'g_secretId.c_str(), g_localUserName.c_str(), [self getLocalColorInt]];')

# 7. Replace chat color
chat_old = 'std::string json = "{\\"type\\":\\"chat\\", \\"msg\\":\\"" + [msg UTF8String] + "\\", \\"nclr\\":0}\\n";'
chat_new = 'std::string json = std::string("{\\"type\\":\\"chat\\", \\"msg\\":\\"") + [msg UTF8String] + "\\", \\"nclr\\":" + std::to_string([self getLocalColorInt]) + "}\\n";'
text = text.replace(chat_old, chat_new)

# 8. menuNeedsUpdate: Add IDS_WATCH_VIDEO
menu_old = '[menu addItemWithTitle:NSLocalizedString(@"IDS_REPORT_SPAM", nil) action:@selector(onReportClicked:) keyEquivalent:@""];'
menu_new = menu_old + """
            bool isStreaming = (g_videoStreamers.find(uid) != g_videoStreamers.end() && (GetTickCountMs() - g_videoStreamers[uid] < 5000));
            if (isStreaming) {
                NSMenuItem *watchItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"IDS_WATCH_VIDEO", nil) action:@selector(onWatchVideoClicked:) keyEquivalent:@""];
                watchItem.representedObject = [NSString stringWithUTF8String:uid.c_str()];
                [menu addItem:watchItem];
            }"""
text = text.replace(menu_old, menu_new)

# 9. Implement onWatchVideoClicked: and onBlockClicked: logic inside AppDelegate
watch_func = """- (void)onWatchVideoClicked:(id)sender {
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
            [self appendSystemLogMessage:[NSString stringWithFormat:@"Blocked user: %s", g_userList[row].userName.c_str()]];
        } else {
            g_ignoredUsers.erase(uid);
            [self appendSystemLogMessage:[NSString stringWithFormat:@"Unblocked user: %s", g_userList[row].userName.c_str()]];
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
            [self appendSystemLogMessage:[NSString stringWithFormat:@"Reported user: %s", g_userList[row].userName.c_str()]];
        }
    }
}
"""
# Replace empty block and report clicked
text = re.sub(r'- \(void\)onBlockClicked:\(id\)sender \{\n\}', watch_func, text)
text = re.sub(r'- \(void\)onReportClicked:\(id\)sender \{\n\}', '', text)

with open('main.mm', 'w') as f:
    f.write(text)
print("Done!")
