#import <AppKit/AppKit.h>
#include <iostream>
#include <string>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <AVFoundation/AVFoundation.h>
#import <CommonCrypto/CommonDigest.h>
#include <iostream>
#include <map>
#include <vector>
#include <memory>
#include <set>
@class VideoWindowController;

#include "ChatNetwork.h"
#include "MacAudioEngine.h"
#include "MacVideoEngine.h"

// Global State
static std::string g_secretId = "MacLocalSecret-12345";
static std::string g_localUserName = "MacUser";
std::string g_userId = "";
static int g_captureSourceType = 0; // 0=None, 1=Camera, 2=Screen, 3=Window
static uint32_t g_captureWindowId = 0;
static std::set<std::string> g_ignoredUsers;
static std::map<std::string, VideoWindowController*> g_videoWindows;
static std::map<std::string, uint32_t> g_videoStreamers;

static uint32_t GetTickCountMs() {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

std::string HashRoomName(NSString *roomName) {
    const char *utf8Str = [roomName UTF8String];
    unsigned char hash[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(utf8Str, (CC_LONG)strlen(utf8Str), hash);
    
    char hex[3];
    std::string result = "";
    for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) {
        sprintf(hex, "%02x", hash[i]);
        result += hex;
    }
    return "private_" + result;
}

struct UserInfo {
    std::string userId;
    std::string userName;
    int color;
};
static std::vector<UserInfo> g_userList;

// --- Window Snapping Subclass ---
@interface SnappableWindow : NSWindow
@property (assign) NSRect unsnappedFrame;
@property (assign) BOOL isProgrammaticSet;
@end

@implementation SnappableWindow
- (void)setFrame:(NSRect)frameRect display:(BOOL)flag {
    if (self.isProgrammaticSet || NSEvent.pressedMouseButtons == 0) {
        self.unsnappedFrame = NSZeroRect;
        [super setFrame:frameRect display:flag];
        return;
    }
    
    NSRect actualFrame = self.frame;
    if (!NSEqualSizes(frameRect.size, actualFrame.size)) {
        self.unsnappedFrame = NSZeroRect;
        [super setFrame:frameRect display:flag];
        return;
    }
    
    CGFloat dx = frameRect.origin.x - actualFrame.origin.x;
    CGFloat dy = frameRect.origin.y - actualFrame.origin.y;
    
    if (NSEqualRects(self.unsnappedFrame, NSZeroRect)) {
        self.unsnappedFrame = actualFrame;
    }
    
    NSRect newUnsnapped = self.unsnappedFrame;
    newUnsnapped.origin.x += dx;
    newUnsnapped.origin.y += dy;
    self.unsnappedFrame = newUnsnapped;
    
    NSRect snappedFrame = newUnsnapped;
    NSRect screenFrame = self.screen.visibleFrame;
    CGFloat snapEdge = 16.0;
    CGFloat snapCorner = 32.0;
    
    CGFloat dLeft = newUnsnapped.origin.x - screenFrame.origin.x;
    CGFloat dRight = (screenFrame.origin.x + screenFrame.size.width) - (newUnsnapped.origin.x + newUnsnapped.size.width);
    CGFloat dBottom = newUnsnapped.origin.y - screenFrame.origin.y;
    CGFloat dTop = (screenFrame.origin.y + screenFrame.size.height) - (newUnsnapped.origin.y + newUnsnapped.size.height);
    
    CGFloat x_cands[] = { dLeft, -dRight };
    CGFloat y_cands[] = { dBottom, -dTop };
    
    CGFloat minDx = 9999, minDy = 9999;
    BOOL cornerSnapped = NO;
    
    for (int i=0; i<2; i++) {
        for (int j=0; j<2; j++) {
            if (ABS(x_cands[i]) < snapCorner && ABS(y_cands[j]) < snapCorner) {
                if (ABS(x_cands[i]) + ABS(y_cands[j]) < ABS(minDx) + ABS(minDy)) {
                    minDx = x_cands[i];
                    minDy = y_cands[j];
                    cornerSnapped = YES;
                }
            }
        }
    }
    
    if (cornerSnapped) {
        snappedFrame.origin.x -= minDx;
        snappedFrame.origin.y -= minDy;
    } else {
        minDx = 9999; minDy = 9999;
        for (int i=0; i<2; i++) {
            if (ABS(x_cands[i]) < snapEdge && ABS(x_cands[i]) < ABS(minDx)) {
                minDx = x_cands[i];
            }
        }
        for (int j=0; j<2; j++) {
            if (ABS(y_cands[j]) < snapEdge && ABS(y_cands[j]) < ABS(minDy)) {
                minDy = y_cands[j];
            }
        }
    }
    
    if (cornerSnapped) {
        snappedFrame.origin.x -= minDx;
        snappedFrame.origin.y -= minDy;
    }
    
    [super setFrame:snappedFrame display:flag];
    self.isProgrammaticSet = NO;
}
@end



@interface VideoWindowController : NSWindowController <NSWindowDelegate>
@property (strong) AVSampleBufferDisplayLayer *videoLayer;
@property (strong) NSImageView *jpegView;
@property (strong) NSString *userId;
- (instancetype)initWithUserId:(NSString *)userId userName:(NSString*)userName;
- (void)processVideoData:(const uint8_t*)data size:(size_t)size isH264:(bool)isH264;
@end

@interface UserIconView : NSView
@property (assign) BOOL isIgnored;
@property (assign) BOOL isStreaming;
@end

@implementation UserIconView
- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    NSRect bounds = self.bounds;
    CGFloat iconSize = 18.0;
    CGFloat cx = (bounds.size.width - iconSize) / 2.0;
    CGFloat cy = (bounds.size.height - iconSize) / 2.0;
    
    if (self.isStreaming) {
        // Retro TV cabinet (Wood brown)
        NSRect tvRect = NSMakeRect(cx - 2, cy - 2, iconSize + 4, iconSize + 4);
        [[NSColor colorWithCalibratedRed:0.54 green:0.27 blue:0.07 alpha:1.0] setFill];
        NSRectFill(tvRect);
        
        [[NSColor colorWithCalibratedRed:0.3 green:0.15 blue:0.05 alpha:1.0] setStroke];
        [NSBezierPath strokeRect:tvRect];
        
        // TV Bezel / Screen
        NSRect screenRect = NSMakeRect(cx - 1, cy - 1, iconSize + 2, iconSize + 2);
        [[NSColor colorWithCalibratedRed:0.15 green:0.15 blue:0.15 alpha:1.0] setFill];
        NSRectFill(screenRect);
        
        // Antenna
        NSBezierPath *ant = [NSBezierPath bezierPath];
        CGFloat antX = cx + iconSize / 2.0;
        CGFloat antY = cy + iconSize + 2.0;
        [ant moveToPoint:NSMakePoint(antX, antY)];
        [ant lineToPoint:NSMakePoint(antX - 4, antY + 4)];
        [ant moveToPoint:NSMakePoint(antX, antY)];
        [ant lineToPoint:NSMakePoint(antX + 4, antY + 4)];
        [[NSColor colorWithCalibratedRed:0.6 green:0.6 blue:0.6 alpha:1.0] setStroke];
        [ant setLineWidth:1.5];
        [ant stroke];
    }
    
    // Face circle
    NSRect faceRect = NSMakeRect(cx, cy, iconSize, iconSize);
    NSBezierPath *facePath = [NSBezierPath bezierPathWithOvalInRect:faceRect];
    
    if (self.isIgnored) {
        [[NSColor colorWithCalibratedRed:0.7 green:0.7 blue:0.7 alpha:1.0] setFill];
        [facePath fill];
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] setStroke];
        [facePath stroke];
    } else {
        [[NSColor colorWithCalibratedRed:1.0 green:0.84 blue:0.0 alpha:1.0] setFill]; // Gold / Yellow
        [facePath fill];
        [[NSColor colorWithCalibratedRed:0.9 green:0.6 blue:0.0 alpha:1.0] setStroke]; // Orange border
        [facePath stroke];
    }
    
    // Eyes
    NSColor *eyeColor = self.isIgnored ? [NSColor colorWithCalibratedRed:0.3 green:0.3 blue:0.3 alpha:1.0] : [NSColor blackColor];
    [eyeColor setFill];
    
    NSRect leftEye = NSMakeRect(cx + 4.5, cy + iconSize - 7.5, 3.0, 3.0);
    NSRect rightEye = NSMakeRect(cx + iconSize - 7.5, cy + iconSize - 7.5, 3.0, 3.0);
    [[NSBezierPath bezierPathWithOvalInRect:leftEye] fill];
    [[NSBezierPath bezierPathWithOvalInRect:rightEye] fill];
    
    // Smile mouth
    NSBezierPath *mouth = [NSBezierPath bezierPath];
    [mouth appendBezierPathWithArcWithCenter:NSMakePoint(cx + iconSize / 2.0, cy + iconSize / 2.0 + 1.0)
                                      radius:4.5
                                  startAngle:200
                                    endAngle:340
                                   clockwise:NO];
    [eyeColor setStroke];
    [mouth setLineWidth:1.5];
    [mouth stroke];
}
@end

@interface UserTableCellView : NSTableCellView
@property (strong) UserIconView *iconView;
@property (strong) NSTextField *nameLabel;
@end

@implementation UserTableCellView
- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        self.iconView = [[UserIconView alloc] initWithFrame:NSMakeRect(2, 2, 24, 24)];
        [self addSubview:self.iconView];
        
        self.nameLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(28, 2, frameRect.size.width - 30, 24)];
        self.nameLabel.editable = NO;
        self.nameLabel.bordered = NO;
        self.nameLabel.drawsBackground = NO;
        self.nameLabel.autoresizingMask = NSViewWidthSizable;
        [self addSubview:self.nameLabel];
    }
    return self;
}
@end

@interface MoveRoomWindowController : NSWindowController <NSTableViewDataSource, NSTableViewDelegate, NSTextFieldDelegate>
@property (strong) NSTextField *roomField;
@property (strong) NSTableView *roomTableView;
@property (strong) NSButton *privateCheckbox;
@property (strong) NSButton *okButton;
@property (strong) NSButton *cancelButton;
@property (assign) BOOL isOK;
@property (strong) NSString *selectedRoom;
@property (assign) BOOL isPrivate;
@property (strong) NSMutableArray<NSDictionary*> *allRooms;
@property (strong) NSMutableArray<NSDictionary*> *filteredRooms;
@property (strong) NSTimer *refreshTimer;
- (instancetype)initWithCurrentRoom:(NSString *)currentRoom;
- (void)updatePublicRooms:(NSArray *)rooms;
@end

@implementation MoveRoomWindowController

- (instancetype)initWithCurrentRoom:(NSString *)currentRoom {
    NSRect frame = NSMakeRect(0, 0, 300, 270);
    NSWindow *win = [[NSWindow alloc] initWithContentRect:frame
                                                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                  backing:NSBackingStoreBuffered
                                                    defer:NO];
    [win setTitle:NSLocalizedString(@"IDS_MOVE_ROOM_TITLE", nil)];
    win.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
    self = [super initWithWindow:win];
    if (self) {
        self.allRooms = [NSMutableArray array];
        self.filteredRooms = [NSMutableArray array];
        NSView *content = win.contentView;
        
        self.selectedRoom = currentRoom ? currentRoom : NSLocalizedString(@"IDS_LOBBY", nil);
        self.roomField = [[NSTextField alloc] initWithFrame:NSMakeRect(15, 230, 270, 24)];
        self.roomField.stringValue = @"";
        self.roomField.placeholderString = NSLocalizedString(@"IDS_MOVE_ROOM_TITLE", nil);
        self.roomField.delegate = self;
        [content addSubview:self.roomField];
        
        NSScrollView *scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(15, 50, 270, 170)];
        scroll.hasVerticalScroller = YES;
        scroll.autohidesScrollers = YES;
        
        self.roomTableView = [[NSTableView alloc] initWithFrame:scroll.bounds];
        NSTableColumn *col = [[NSTableColumn alloc] initWithIdentifier:@"RoomCol"];
        col.width = 250;
        [self.roomTableView addTableColumn:col];
        self.roomTableView.headerView = nil;
        self.roomTableView.dataSource = self;
        self.roomTableView.delegate = self;
        self.roomTableView.target = self;
        self.roomTableView.doubleAction = @selector(onDoubleClickRow:);
        scroll.documentView = self.roomTableView;
        [content addSubview:scroll];
        
        self.privateCheckbox = [[NSButton alloc] initWithFrame:NSMakeRect(15, 15, 90, 25)];
        [self.privateCheckbox setButtonType:NSButtonTypeSwitch];
        self.privateCheckbox.title = NSLocalizedString(@"IDS_PRIVATE", nil);
        self.privateCheckbox.state = NSControlStateValueOff;
        [content addSubview:self.privateCheckbox];
        
        self.okButton = [[NSButton alloc] initWithFrame:NSMakeRect(135, 12, 70, 30)];
        self.okButton.title = @"OK";
        self.okButton.bezelStyle = NSBezelStyleRounded;
        self.okButton.target = self;
        self.okButton.action = @selector(onOK:);
        [content addSubview:self.okButton];
        
        self.cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(210, 12, 75, 30)];
        self.cancelButton.title = NSLocalizedString(@"IDS_CANCEL", nil);
        self.cancelButton.bezelStyle = NSBezelStyleRounded;
        self.cancelButton.target = self;
        self.cancelButton.action = @selector(onCancel:);
        [content addSubview:self.cancelButton];
        self.refreshTimer = [NSTimer scheduledTimerWithTimeInterval:5.0 target:self selector:@selector(onTimer:) userInfo:nil repeats:YES];
    }
    return self;
}

- (void)onTimer:(NSTimer *)timer {
    extern class ChatNetwork *g_chatNetworkInstance;
    [[NSNotificationCenter defaultCenter] postNotificationName:@"RequestRoomListNotification" object:nil];
}
- (void)updatePublicRooms:(NSArray *)rooms {
    NSLog(@"updatePublicRooms called with %lu rooms", (unsigned long)rooms.count);
    [self.allRooms removeAllObjects];
    for (id r in rooms) {
        NSString *name = nil;
        int count = 1;
        if ([r isKindOfClass:[NSDictionary class]]) {
            name = r[@"name"];
            if (r[@"count"]) count = [r[@"count"] intValue];
        } else if ([r isKindOfClass:[NSString class]]) {
            name = (NSString *)r;
        }
        
        if (name && name.length > 0) {
            [self.allRooms addObject:@{@"display": [NSString stringWithFormat:@"%@ (%d)", name, count], @"name": name}];
        }
    }
    NSLog(@"allRooms count is now %lu", (unsigned long)self.allRooms.count);
    [self filterRooms];
}


- (void)controlTextDidChange:(NSNotification *)obj {
    [self filterRooms];
}

- (void)filterRooms {
    [self.filteredRooms removeAllObjects];
    NSString *query = self.roomField.stringValue.lowercaseString;
    for (NSDictionary *r in self.allRooms) {
        NSString *name = r[@"name"];
        if (query.length == 0 || [name.lowercaseString containsString:query]) {
            [self.filteredRooms addObject:r];
        }
    }
    NSLog(@"filterRooms finished, filteredRooms count: %lu", (unsigned long)self.filteredRooms.count);
    [self.roomTableView reloadData];
    [self.roomTableView setNeedsDisplay:YES];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    NSLog(@"numberOfRowsInTableView called, returning %lu", (unsigned long)self.filteredRooms.count);
    return self.filteredRooms.count;
}


- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    NSLog(@"tableView:viewForTableColumn:row: called for row %ld", (long)row);
    NSTextField *cell = [tableView makeViewWithIdentifier:@"RoomCell" owner:self];
    if (!cell) {
        cell = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 250, 20)];
        cell.identifier = @"RoomCell";
        cell.editable = NO;
        cell.bordered = NO;
        cell.drawsBackground = NO;
        cell.font = [NSFont systemFontOfSize:13];
        cell.textColor = [NSColor labelColor];
    }
    if (row < self.filteredRooms.count) {
        cell.stringValue = self.filteredRooms[row][@"display"];
    }
    return cell;
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
    NSInteger sel = self.roomTableView.selectedRow;
    if (sel >= 0 && sel < self.filteredRooms.count) {
        self.roomField.stringValue = self.filteredRooms[sel][@"name"];
    }
}

- (void)onDoubleClickRow:(id)sender {
    NSInteger sel = self.roomTableView.clickedRow;
    if (sel >= 0 && sel < self.filteredRooms.count) {
        self.roomField.stringValue = self.filteredRooms[sel][@"name"];
        [self onOK:nil];
    }
}

- (void)onOK:(id)sender {
    [self.refreshTimer invalidate];
    self.refreshTimer = nil;
    self.isOK = YES;
    NSString *room = self.roomField.stringValue;
    if (room.length == 0) {
        room = self.selectedRoom ? self.selectedRoom : NSLocalizedString(@"IDS_LOBBY", nil);
    }
    self.selectedRoom = room;
    self.isPrivate = (self.privateCheckbox.state == NSControlStateValueOn);
    [NSApp stopModalWithCode:NSModalResponseOK];
    [self.window close];
}

- (void)onCancel:(id)sender {
    [self.refreshTimer invalidate];
    self.refreshTimer = nil;
    self.isOK = NO;
    [NSApp stopModalWithCode:NSModalResponseCancel];
    [self.window close];
}
@end

static NSArray *g_lastPublicRooms = nil;
static MoveRoomWindowController *g_activeMoveRoomDlg = nil;

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate, NSTableViewDataSource, NSTableViewDelegate, NSMenuDelegate, NSTextViewDelegate, NSComboBoxDelegate> {
    std::unique_ptr<ChatNetwork> chatNetwork;
    std::unique_ptr<MacAudioEngine> audioEngine;
    std::unique_ptr<MacVideoEngine> videoEngine;
}
@property (strong) SnappableWindow *window;
@property (strong) NSScrollView *logScroll;
@property (strong) NSTextView *logTextView;
@property (strong) NSScrollView *inputScroll;
@property (strong) NSTextView *inputTextView;
@property (strong) NSTextField *userListHeader;
@property (strong) NSScrollView *userScroll;
@property (strong) NSTableView *userListView;
@property (strong) NSButton *sendButton;
@property (strong) NSButton *voiceButton;

@property (strong) NSButton *btnB;
@property (strong) NSButton *btnI;
@property (strong) NSButton *btnU;
@property (strong) NSComboBox *sizeCombo;
@property (strong) NSColorWell *colorWell;
@property (strong) NSButton *moveRoomBtn;
@property (strong) NSString *currentRoomName;
@property (strong) NSLevelIndicator *micVolumeIndicator;
@property (strong) NSSlider *volumeSlider;
@property (strong) NSTextField *maintenanceView;
- (void)fetchMaintenanceMessage;
- (void)updateUserListUI;
- (void)layoutControls;
- (void)onMoveRoomClicked:(id)sender;
- (void)onShowRenameDialog:(id)sender;
- (void)onFormatChanged:(id)sender;
- (void)updateWindowTitle;
@end

@implementation VideoWindowController {
    std::unique_ptr<MacVideoEngine> decoderEngine;
}
- (instancetype)initWithUserId:(NSString *)userId userName:(NSString*)userName {
    NSRect frame = NSMakeRect(0, 0, 320, 180);
    NSWindow *win = [[NSWindow alloc] initWithContentRect:frame
                                                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                  backing:NSBackingStoreBuffered
                                                    defer:NO];
    [win setTitle:userName];
    [win center];
    win.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
    self = [super initWithWindow:win];
    if (self) {
        self.userId = userId;
        win.delegate = self;
        
        NSView *contentView = win.contentView;
        contentView.wantsLayer = YES;
        contentView.layer.backgroundColor = [[NSColor blackColor] CGColor];
        
        self.videoLayer = [[AVSampleBufferDisplayLayer alloc] init];
        self.videoLayer.frame = contentView.bounds;
        self.videoLayer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
        self.videoLayer.videoGravity = AVLayerVideoGravityResizeAspect;
        [contentView.layer addSublayer:self.videoLayer];
        
        self.jpegView = [[NSImageView alloc] initWithFrame:contentView.bounds];
        self.jpegView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        self.jpegView.imageScaling = NSImageScaleProportionallyUpOrDown;
        self.jpegView.hidden = YES;
        [contentView addSubview:self.jpegView];
        
        __weak typeof(self) weakSelf = self;
        decoderEngine = std::make_unique<MacVideoEngine>();
        decoderEngine->InitializeDecoder();
        decoderEngine->SetDecodedCallback([weakSelf](void* sampleBuffer) {
            CMSampleBufferRef sbuf = (CMSampleBufferRef)sampleBuffer;
            if (sbuf) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    VideoWindowController *strongSelf = weakSelf;
                    if (strongSelf) {
                        if (strongSelf.videoLayer.status == AVQueuedSampleBufferRenderingStatusFailed) {
                            [strongSelf.videoLayer flush];
                        }
                        [strongSelf.videoLayer enqueueSampleBuffer:sbuf];
                    }
                });
            }
        });
    }
    return self;
}

- (void)processVideoData:(const uint8_t*)data size:(size_t)size isH264:(bool)isH264 {
    if (isH264) {
        decoderEngine->DecodeFrame(data, size);
        dispatch_async(dispatch_get_main_queue(), ^{
            self.jpegView.hidden = YES;
            self.videoLayer.hidden = NO;
        });
    } else {
        NSData *nsData = [NSData dataWithBytes:data length:size];
        NSImage *image = [[NSImage alloc] initWithData:nsData];
        if (image) {
            dispatch_async(dispatch_get_main_queue(), ^{
                self.jpegView.image = image;
                self.videoLayer.hidden = YES;
                self.jpegView.hidden = NO;
            });
        }
    }
}

- (void)windowWillClose:(NSNotification *)notification {
    std::string uid = [self.userId UTF8String];
    auto it = g_videoWindows.find(uid);
    if (it != g_videoWindows.end()) {
        g_videoWindows.erase(it);
    }
}
@end

@implementation AppDelegate

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
                while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ' || line.back() == '\t')) {
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
                        
                        NSString *roomName = strongSelf.currentRoomName ? strongSelf.currentRoomName : NSLocalizedString(@"IDS_LOBBY", nil);
                        NSString *localName = [NSString stringWithUTF8String:g_localUserName.c_str()];
                        NSRange hashRange = [localName rangeOfString:@"#"];
                        NSString *dispName = (hashRange.location != NSNotFound) ? [localName substringToIndex:hashRange.location] : localName;
                        [strongSelf appendSystemLogMessage:[NSString stringWithFormat:NSLocalizedString(@"IDS_ROOM_ENTER_FMT", nil), dispName, roomName]];
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
                        if (!found) {
                            g_userList.push_back(info);
                            NSString *roomName = strongSelf.currentRoomName ? strongSelf.currentRoomName : NSLocalizedString(@"IDS_LOBBY", nil);
                            NSString *userNameStr = dict[@"userName"] ? dict[@"userName"] : @"Unknown";
                            NSRange hashRange = [userNameStr rangeOfString:@"#"];
                            NSString *dispName = (hashRange.location != NSNotFound) ? [userNameStr substringToIndex:hashRange.location] : userNameStr;
                            NSString *logMsg = [NSString stringWithFormat:NSLocalizedString(@"IDS_ROOM_ENTER_FMT", nil), dispName, roomName];
                            [strongSelf appendSystemLogMessage:logMsg];
                        }
                        [strongSelf updateUserListUI];
                    } else if ([type isEqualToString:@"leave"]) {
                        std::string uid = [dict[@"userId"] UTF8String];
                        NSString *userNameStr = @"Unknown";
                        for (const auto& u : g_userList) {
                            if (u.userId == uid) {
                                userNameStr = [NSString stringWithUTF8String:u.userName.c_str()];
                                break;
                            }
                        }
                        g_userList.erase(std::remove_if(g_userList.begin(), g_userList.end(), [&](const UserInfo& u) { return u.userId == uid; }), g_userList.end());
                        NSString *roomName = strongSelf.currentRoomName ? strongSelf.currentRoomName : NSLocalizedString(@"IDS_LOBBY", nil);
                        NSRange hashRange = [userNameStr rangeOfString:@"#"];
                        NSString *dispName = (hashRange.location != NSNotFound) ? [userNameStr substringToIndex:hashRange.location] : userNameStr;
                        NSString *logMsg = [NSString stringWithFormat:NSLocalizedString(@"IDS_ROOM_LEAVE_FMT", nil), dispName, roomName];
                        [strongSelf appendSystemLogMessage:logMsg];
                        [strongSelf updateUserListUI];
                    } else if ([type isEqualToString:@"rename"]) {
                        std::string uid = [dict[@"target"] UTF8String];
                        std::string newName = [dict[@"name"] UTF8String];
                        for (auto& u : g_userList) {
                            if (u.userId == uid) {
                                u.userName = newName;
                                if (dict[@"nclr"]) {
                                    u.color = [dict[@"nclr"] intValue];
                                }
                                break;
                            }
                        }
                        [strongSelf updateUserListUI];
                    } else if ([type isEqualToString:@"join_resp"] || [type isEqualToString:@"welcome"]) {
                        NSString *uid = dict[@"userId"];
                        if (uid) g_userId = [uid UTF8String];
                        if (strongSelf->chatNetwork) strongSelf->chatNetwork->SendMessage("{\"type\":\"get_rooms\"}\n");
                    } else if ([type isEqualToString:@"redirect"]) {
                        NSString *ip = dict[@"ip"];
                        int port = [dict[@"port"] intValue];
                        NSString *newRoom = dict[@"roomName"];
                        if (ip) {
                            std::string newIp = [ip UTF8String];
                            if (strongSelf->chatNetwork) {
                                if (newRoom) {
                                    strongSelf.currentRoomName = newRoom;
                                    [strongSelf updateWindowTitle];
                                    [strongSelf appendSystemLogMessage:[NSString stringWithFormat:NSLocalizedString(@"IDS_MOVED_TO", nil), newRoom]];
                                }
                                strongSelf->chatNetwork->Connect(newIp, port, 4001);
                                NSString *room = strongSelf.currentRoomName ? strongSelf.currentRoomName : NSLocalizedString(@"IDS_LOBBY", nil);
                                NSString *rejoinMsg = [NSString stringWithFormat:@"{\"type\":\"join\", \"secretId\":\"%s\", \"userName\":\"%s\", \"roomName\":\"%@\", \"pub\":true, \"nclr\":%d}\n",
                                                       g_secretId.c_str(), g_localUserName.c_str(), room, [strongSelf getLocalColorInt]];
                                strongSelf->chatNetwork->SendMessage([rejoinMsg UTF8String]);
                            }
                        }
                    } else if ([type isEqualToString:@"rooms"] || [type isEqualToString:@"room_list"]) {
                        NSLog(@"Room list received: %@", dict);
                        NSArray *rooms = dict[@"rooms"];
                        if (rooms) {
                            g_lastPublicRooms = rooms;
                            NSLog(@"g_activeMoveRoomDlg = %@", g_activeMoveRoomDlg);
                            if (g_activeMoveRoomDlg) {
                                [g_activeMoveRoomDlg updatePublicRooms:rooms];
                            }
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
                        NSString *rawName = dict[@"userName"] ? dict[@"userName"] : (dict[@"user"] ? dict[@"user"] : @"");
                        int nclr = dict[@"nclr"] ? [dict[@"nclr"] intValue] : 0xFFFFFFFF;
                        [strongSelf appendLogWithUser:rawName message:msg ? msg : @"" nameColor:nclr textColor:textColor font:font isSystem:NO isUnderline:u];
                    }
                }
            }
        });
        CFRunLoopWakeUp(CFRunLoopGetMain());
    });

    NSRect frame = NSMakeRect(0, 0, 800, 600);
    NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
    self.window = [[SnappableWindow alloc] initWithContentRect:frame styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
    self.window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
    [self.window setTitle:@"MacChat"];
    [self.window center];
    self.window.delegate = self;
    
    self.logScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    self.logScroll.hasVerticalScroller = YES;
    NSSize contentSize = [self.logScroll contentSize];
    self.logTextView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, contentSize.width, contentSize.height)];
    self.logTextView.minSize = NSMakeSize(0.0, contentSize.height);
    self.logTextView.maxSize = NSMakeSize(CGFLOAT_MAX, CGFLOAT_MAX);
    self.logTextView.verticallyResizable = YES;
    self.logTextView.horizontallyResizable = NO;
    self.logTextView.autoresizingMask = NSViewWidthSizable;
    [[self.logTextView textContainer] setContainerSize:NSMakeSize(contentSize.width, CGFLOAT_MAX)];
    [[self.logTextView textContainer] setWidthTracksTextView:YES];
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
    self.btnB.bezelStyle = NSBezelStyleRegularSquare;
    self.btnB.title = @"B";
    self.btnB.target = self;
    self.btnB.action = @selector(onFormatChanged:);
    [self.window.contentView addSubview:self.btnB];
    
    self.btnI = [[NSButton alloc] initWithFrame:NSZeroRect];
    [self.btnI setButtonType:NSButtonTypePushOnPushOff];
    self.btnI.bezelStyle = NSBezelStyleRegularSquare;
    self.btnI.title = @"I";
    self.btnI.target = self;
    self.btnI.action = @selector(onFormatChanged:);
    [self.window.contentView addSubview:self.btnI];
    
    self.btnU = [[NSButton alloc] initWithFrame:NSZeroRect];
    [self.btnU setButtonType:NSButtonTypePushOnPushOff];
    self.btnU.bezelStyle = NSBezelStyleRegularSquare;
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
    self.moveRoomBtn.title = NSLocalizedString(@"IDS_MOVE_ROOM_BTN", nil);
    self.moveRoomBtn.target = self;
    self.moveRoomBtn.action = @selector(onMoveRoomClicked:);
    [self.window.contentView addSubview:self.moveRoomBtn];
    
    self.voiceButton = [[NSButton alloc] initWithFrame:NSZeroRect];
    self.voiceButton.title = [NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_TALK", nil)];
    self.voiceButton.target = self;
    self.voiceButton.action = @selector(onVoiceClicked:);
    [self.window.contentView addSubview:self.voiceButton];
    
    
    self.micVolumeIndicator = [[NSLevelIndicator alloc] initWithFrame:NSZeroRect];
    self.micVolumeIndicator.levelIndicatorStyle = NSLevelIndicatorStyleContinuousCapacity;
    [self.window.contentView addSubview:self.micVolumeIndicator];
    
    self.volumeSlider = [[NSSlider alloc] initWithFrame:NSZeroRect];
    self.volumeSlider.doubleValue = 1.0;
    self.volumeSlider.target = self;
    self.volumeSlider.action = @selector(onVolumeSliderChanged:);
    [self.window.contentView addSubview:self.volumeSlider];
    
    self.maintenanceView = [[NSTextField alloc] initWithFrame:NSZeroRect];
    self.maintenanceView.hidden = YES;
    self.maintenanceView.editable = NO;
    self.maintenanceView.bordered = NO;
    self.maintenanceView.drawsBackground = NO;
    [self.window.contentView addSubview:self.maintenanceView];
    
    self.inputScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    self.inputScroll.hasVerticalScroller = YES;
    NSSize inputContentSize = [self.inputScroll contentSize];
    self.inputTextView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, inputContentSize.width, inputContentSize.height)];
    self.inputTextView.minSize = NSMakeSize(0.0, inputContentSize.height);
    self.inputTextView.maxSize = NSMakeSize(CGFLOAT_MAX, CGFLOAT_MAX);
    self.inputTextView.verticallyResizable = YES;
    self.inputTextView.horizontallyResizable = NO;
    self.inputTextView.autoresizingMask = NSViewWidthSizable;
    [[self.inputTextView textContainer] setContainerSize:NSMakeSize(inputContentSize.width, CGFLOAT_MAX)];
    [[self.inputTextView textContainer] setWidthTracksTextView:YES];
    self.inputTextView.editable = YES;
    self.inputTextView.richText = YES;
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
        NSString *joinMsg = [NSString stringWithFormat:@"{\"type\":\"join\", \"secretId\":\"%s\", \"userName\":\"%s\", \"roomName\":\"Lobby\", \"pub\":true, \"nclr\":%d}\n",
                             g_secretId.c_str(), g_localUserName.c_str(), [self getLocalColorInt]];
        chatNetwork->SendMessage([joinMsg UTF8String]);
        chatNetwork->SendMessage("{\"type\":\"get_rooms\"}\n");
        NSString *localName = [NSString stringWithUTF8String:g_localUserName.c_str()];
        NSRange hashRange = [localName rangeOfString:@"#"];
        NSString *dispName = (hashRange.location != NSNotFound) ? [localName substringToIndex:hashRange.location] : localName;
        [self appendSystemLogMessage:[NSString stringWithFormat:@"%@ (%@)", NSLocalizedString(@"IDS_JOINED_NETWORK", nil), dispName]];
        [self appendSystemLogMessage:[NSString stringWithFormat:NSLocalizedString(@"IDS_MOVED_TO", nil), NSLocalizedString(@"IDS_LOBBY", nil)]];
    }
    
    [self fetchMaintenanceMessage];
    [NSTimer scheduledTimerWithTimeInterval:3600 repeats:YES block:^(NSTimer * _Nonnull timer) {
        [self fetchMaintenanceMessage];
    }];
}
- (void)layoutControls {
    if (!self.window) return;
    
    NSRect bounds = self.window.contentView.bounds;
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
    
    self.voiceButton.frame = NSMakeRect(currentX, toolY, 75, toolHeight);
    currentX += 80;
    
    self.micVolumeIndicator.frame = NSMakeRect(currentX, toolY + (toolHeight - 16) / 2, 80, 16);
    currentX += 85;
    
    self.volumeSlider.frame = NSMakeRect(currentX, toolY + (toolHeight - 20) / 2, 90, 18);
    currentX += 90 + gap;
    
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
}

- (void)updateWindowTitle {
    NSString *title = [NSString stringWithFormat:@"MacChat - [%@]", self.currentRoomName];
    [self.window setTitle:title];
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
    if (!font) font = [NSFont systemFontOfSize:fontSize];
    
    [self.inputTextView setFont:font];
    [self.inputTextView setTextColor:self.colorWell.color];
    
    NSMutableDictionary *attrs = [NSMutableDictionary dictionary];
    attrs[NSFontAttributeName] = font;
    attrs[NSForegroundColorAttributeName] = self.colorWell.color;
    if (self.btnU.state == NSControlStateValueOn) {
        attrs[NSUnderlineStyleAttributeName] = @(NSUnderlineStyleSingle);
    }
    
    [self.inputTextView.textStorage addAttributes:attrs range:NSMakeRange(0, self.inputTextView.string.length)];
    self.inputTextView.typingAttributes = attrs;
}

- (int)getLocalColorInt {
    NSColor *color = [self.colorWell.color colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
    if (!color) return 0;
    int r = (int)(color.redComponent * 255.0) & 0xFF;
    int g = (int)(color.greenComponent * 255.0) & 0xFF;
    int b = (int)(color.blueComponent * 255.0) & 0xFF;
    return r | (g << 8) | (b << 16);
}

- (void)enforceLogLimit {
    NSTextStorage *storage = [self.logTextView textStorage];
    NSString *string = [storage string];
    if (string.length == 0) return;
    NSUInteger numberOfLines = 0;
    NSUInteger index = 0;
    while (index < string.length) {
        index = NSMaxRange([string lineRangeForRange:NSMakeRange(index, 0)]);
        numberOfLines++;
    }
    if (numberOfLines > 5000) {
        NSUInteger linesToDelete = numberOfLines - 5000;
        index = 0;
        for (NSUInteger i = 0; i < linesToDelete && index < string.length; i++) {
            index = NSMaxRange([string lineRangeForRange:NSMakeRange(index, 0)]);
        }
        [storage deleteCharactersInRange:NSMakeRange(0, index)];
    }
}

- (void)appendLogWithUser:(NSString*)user 
                  message:(NSString*)msg 
                nameColor:(int)nclr 
                textColor:(NSColor*)tColor 
                     font:(NSFont*)font 
                 isSystem:(BOOL)isSystem
              isUnderline:(BOOL)isUnderline {
    
    NSMutableAttributedString *finalStr = [[NSMutableAttributedString alloc] init];
    
    if (user.length > 0) {
        NSString *dispName = user;
        NSRange hashRange = [dispName rangeOfString:@"#"];
        if (hashRange.location != NSNotFound) {
            dispName = [dispName substringToIndex:hashRange.location];
        }
        
        NSColor *uColor = [NSColor blackColor];
        if (nclr != -1 && nclr != 0xFFFFFFFF) {
            CGFloat nred = (nclr & 0xFF) / 255.0;
            CGFloat ngreen = ((nclr >> 8) & 0xFF) / 255.0;
            CGFloat nblue = ((nclr >> 16) & 0xFF) / 255.0;
            uColor = [NSColor colorWithCalibratedRed:nred green:ngreen blue:nblue alpha:1.0];
        } else {
            size_t hash = 0;
            for (int i = 0; i < user.length; i++) hash = hash * 31 + [user characterAtIndex:i];
            int palette[] = {
                0x3232C8, 0x329632, 0xC83232, 0x32B4B4, 0xB432B4, 0xB4B432, 0x0080FF, 0xFF0080, 0xFF8000, 0x00C880, 0x8000FF, 0x80C800
            };
            int c = palette[hash % 12];
            uColor = [NSColor colorWithCalibratedRed:(c&0xFF)/255.0 green:((c>>8)&0xFF)/255.0 blue:((c>>16)&0xFF)/255.0 alpha:1.0];
        }
        
        NSFont *nameFont = [NSFont boldSystemFontOfSize:12];
        NSDictionary *nameAttrs = @{ NSFontAttributeName: nameFont, NSForegroundColorAttributeName: uColor };
        
        NSMutableDictionary *msgAttrs = [NSMutableDictionary dictionaryWithDictionary:@{ NSFontAttributeName: font, NSForegroundColorAttributeName: tColor }];
        if (isUnderline) {
            msgAttrs[NSUnderlineStyleAttributeName] = @(NSUnderlineStyleSingle);
        }
        
        if (isSystem) {
            [finalStr appendAttributedString:[[NSAttributedString alloc] initWithString:dispName attributes:nameAttrs]];
            [finalStr appendAttributedString:[[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@"%@\n", msg] attributes:msgAttrs]];
        } else {
            [finalStr appendAttributedString:[[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@"%@: ", dispName] attributes:nameAttrs]];
            [finalStr appendAttributedString:[[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@"%@\n", msg] attributes:msgAttrs]];
        }
    } else {
        NSMutableDictionary *msgAttrs = [NSMutableDictionary dictionaryWithDictionary:@{ NSFontAttributeName: font, NSForegroundColorAttributeName: tColor }];
        if (isUnderline) {
            msgAttrs[NSUnderlineStyleAttributeName] = @(NSUnderlineStyleSingle);
        }
        [finalStr appendAttributedString:[[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@"%@\n", msg] attributes:msgAttrs]];
    }
    
    [[self.logTextView textStorage] appendAttributedString:finalStr];
    [self enforceLogLimit];
    [self.logTextView scrollRangeToVisible:NSMakeRange(self.logTextView.string.length, 0)];
}

- (void)appendLogMessage:(NSString *)message {
    NSFont *font = [NSFont systemFontOfSize:12];
    [self appendLogWithUser:@"" message:message nameColor:0 textColor:[NSColor blackColor] font:font isSystem:NO isUnderline:NO];
}

- (void)appendSystemLogMessage:(NSString *)message {
    NSFont *font = [NSFont systemFontOfSize:12];
    [self appendLogWithUser:@"" message:message nameColor:0 textColor:[NSColor grayColor] font:font isSystem:NO isUnderline:NO];
}

- (void)onSendClicked:(id)sender {
    NSString *msg = self.inputTextView.string;
    if (msg.length > 0 && chatNetwork) {
        bool b = self.btnB.state == NSControlStateValueOn;
        bool i = self.btnI.state == NSControlStateValueOn;
        bool u = self.btnU.state == NSControlStateValueOn;
        int sz = 12;
        NSInteger idx = self.sizeCombo.indexOfSelectedItem;
        if (idx >= 0 && idx < self.sizeCombo.numberOfItems) sz = [[self.sizeCombo itemObjectValueAtIndex:idx] intValue];
        else sz = self.sizeCombo.intValue;
        if (sz <= 0) sz = 12;
        int colorInt = [self getLocalColorInt];
        
        NSMutableString *escapedMsg = [NSMutableString stringWithString:msg];
        [escapedMsg replaceOccurrencesOfString:@"\\" withString:@"\\\\" options:0 range:NSMakeRange(0, escapedMsg.length)];
        [escapedMsg replaceOccurrencesOfString:@"\"" withString:@"\\\"" options:0 range:NSMakeRange(0, escapedMsg.length)];
        [escapedMsg replaceOccurrencesOfString:@"\n" withString:@"\\n" options:0 range:NSMakeRange(0, escapedMsg.length)];
        
        NSString *json = [NSString stringWithFormat:@"{\"type\":\"chat\", \"msg\":\"%@\", \"nclr\":%d, \"b\":%s, \"i\":%s, \"u\":%s, \"sz\":%d, \"clr\":%d}\n",
                          escapedMsg, colorInt, b ? "true" : "false", i ? "true" : "false", u ? "true" : "false", sz, colorInt];
        chatNetwork->SendMessage([json UTF8String]);
        
        // Local Echo
        NSFontTraitMask traits = 0;
        if (b) traits |= NSBoldFontMask;
        if (i) traits |= NSItalicFontMask;
        NSFont *font = [[NSFontManager sharedFontManager] fontWithFamily:@"Helvetica" traits:traits weight:5 size:sz];
        if (!font) font = [NSFont systemFontOfSize:sz];
        CGFloat red = (colorInt & 0xFF) / 255.0;
        CGFloat green = ((colorInt >> 8) & 0xFF) / 255.0;
        CGFloat blue = ((colorInt >> 16) & 0xFF) / 255.0;
        NSColor *textColor = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
        
        NSString *localName = [NSString stringWithUTF8String:g_localUserName.c_str()];
        [self appendLogWithUser:localName message:msg nameColor:colorInt textColor:textColor font:font isSystem:NO isUnderline:u];
        
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

        if (videoEngine) videoEngine->StopCapture();
        if (g_videoWindows.find(g_userId) != g_videoWindows.end()) {
            [g_videoWindows[g_userId] close];
            g_videoWindows.erase(g_userId);
        }
    } else {
        g_captureSourceType = 1;

        if (videoEngine) {
            if (!videoEngine->StartCapture()) {
                NSAlert *alert = [[NSAlert alloc] init];
                [alert setMessageText:@"Camera Permission Required"];
                [alert setInformativeText:@"Please grant Camera permission in System Settings -> Privacy & Security to use video streaming."];
                [alert runModal];
                g_captureSourceType = 0;

            } else {
                [self showLocalVideoWindowWithTitleSuffix:nil];
            }
        }
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

        if (videoEngine) {
            if (!videoEngine->StartScreenCapture()) {
                NSAlert *alert = [[NSAlert alloc] init];
                [alert setMessageText:@"Screen Recording Permission Required"];
                [alert setInformativeText:@"Please grant Screen Recording permission in System Settings -> Privacy & Security to use screen sharing."];
                [alert runModal];
                g_captureSourceType = 0;
            } else {
                [self showLocalVideoWindowWithTitleSuffix:@" (Screen)"];
            }
        }
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

        if (videoEngine) {
            if (!videoEngine->StartWindowCapture(g_captureWindowId)) {
                NSAlert *alert = [[NSAlert alloc] init];
                [alert setMessageText:@"Screen Recording Permission Required"];
                [alert setInformativeText:@"Please grant Screen Recording permission in System Settings -> Privacy & Security to use window sharing."];
                [alert runModal];
                g_captureSourceType = 0;
            } else {
                [self showLocalVideoWindowWithTitleSuffix:@" (Window)"];
            }
        }
    }
}

- (void)menuNeedsUpdate:(NSMenu *)menu {
    [menu removeAllItems];
    NSInteger row = [self.userListView clickedRow];
    if (row >= 0 && row < g_userList.size()) {
        std::string uid = g_userList[row].userId;
        if (uid == g_userId) {
            NSMenuItem *renameItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"IDS_CHANGE_NAME_TITLE", nil) action:@selector(onShowRenameDialog:) keyEquivalent:@""];
            [menu addItem:renameItem];
            
            [menu addItem:[NSMenuItem separatorItem]];
            
            NSMenuItem *camItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"IDS_VIDEO_STREAM", nil) action:@selector(onVideoClicked:) keyEquivalent:@""];
            [camItem setState:(g_captureSourceType == 1) ? NSControlStateValueOn : NSControlStateValueOff];
            [menu addItem:camItem];
            
            NSMenuItem *shareItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"IDS_SCREEN_SHARE", nil) action:nil keyEquivalent:@""];
            NSMenu *shareMenu = [[NSMenu alloc] initWithTitle:NSLocalizedString(@"IDS_SCREEN_SHARE", nil)];
            
            NSMenuItem *fullScreenItem = [[NSMenuItem alloc] initWithTitle:@"Display 1" action:@selector(onScreenContextClicked:) keyEquivalent:@""];
            [fullScreenItem setState:(g_captureSourceType == 2) ? NSControlStateValueOn : NSControlStateValueOff];
            [shareMenu addItem:fullScreenItem];
            
            [shareMenu addItem:[NSMenuItem separatorItem]];
            
            if (@available(macOS 10.15, *)) {
                CGRequestScreenCaptureAccess();
            }
            
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
            NSString *jsonStr = [NSString stringWithFormat:@"{\"type\":\"report\", \"target\":\"%s\"}\n", uid.c_str()];
            chatNetwork->SendMessage([jsonStr UTF8String]);
            [self appendLogMessage:[NSString stringWithFormat:@"Reported user: %s", g_userList[row].userName.c_str()]];
        }
    }
}

- (BOOL)textView:(NSTextView *)textView doCommandBySelector:(SEL)commandSelector {
    if (commandSelector == @selector(insertNewline:)) {
        if ([[NSApp currentEvent] modifierFlags] & NSEventModifierFlagControl) {
            [textView insertText:@"\n" replacementRange:textView.selectedRange];
            return YES;
        } else {
            [self onSendClicked:nil];
            return YES;
        }
    }
    return NO;
}

- (void)fetchMaintenanceMessage {
    NSURL *url = [NSURL URLWithString:@"https://chatannounce2026b9x.blob.core.windows.net/public/announce.json"];
    NSURLRequest *request = [NSURLRequest requestWithURL:url cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:10.0];
    NSURLSessionDataTask *task = [[NSURLSession sharedSession] dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        if (!error && data) {
            NSError *jsonError = nil;
            NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&jsonError];
            if (!jsonError && [json isKindOfClass:[NSDictionary class]]) {
                NSString *lang = [[[NSLocale currentLocale] languageCode] isEqualToString:@"ja"] ? @"ja" : @"en";
                NSString *msg = json[lang];
                if (!msg || msg.length == 0) msg = json[@"en"];
                if (!msg || msg.length == 0) msg = json[@"ja"];
                
                dispatch_async(dispatch_get_main_queue(), ^{
                    if (msg && msg.length > 0) {
                        NSString *htmlStr = [NSString stringWithFormat:@"<html><head><meta charset=\"UTF-8\"></head><body>%@</body></html>", msg];
                        NSData *htmlData = [htmlStr dataUsingEncoding:NSUTF8StringEncoding];
                        NSDictionary *options = @{
                            NSDocumentTypeDocumentAttribute: NSHTMLTextDocumentType,
                            NSCharacterEncodingDocumentOption: @(NSUTF8StringEncoding)
                        };
                        NSAttributedString *attrStr = [[NSAttributedString alloc] initWithData:htmlData options:options documentAttributes:nil error:nil];
                        if (attrStr) {
                            NSMutableAttributedString *mutAttrStr = [attrStr mutableCopy];
                            [mutAttrStr addAttribute:NSFontAttributeName value:[NSFont boldSystemFontOfSize:12] range:NSMakeRange(0, mutAttrStr.length)];
                            [self.maintenanceView setAttributedStringValue:mutAttrStr];
                            self.maintenanceView.hidden = NO;
                        }
                    } else {
                        self.maintenanceView.hidden = YES;
                    }
                    [self layoutControls];
                });
            }
        }
    }];
    [task resume];
}

- (void)updateUserListUI {
    [self.userListView reloadData];
    self.userListHeader.stringValue = [NSString stringWithFormat:NSLocalizedString(@"IDS_USER_LIST_FMT", nil), (int)g_userList.size()];
}

- (void)onMoveRoomClicked:(id)sender {
    MoveRoomWindowController *dlg = [[MoveRoomWindowController alloc] initWithCurrentRoom:self.currentRoomName];
    NSLog(@"Opening MoveRoomWindowController: %@", dlg);
    g_activeMoveRoomDlg = dlg;
    if (chatNetwork) chatNetwork->SendMessage("{\"type\":\"get_rooms\"}\n");
    [NSApp runModalForWindow:dlg.window];
    g_activeMoveRoomDlg = nil;
    if (dlg.isOK) {
        NSString *room = dlg.selectedRoom;
        self.currentRoomName = room;
        NSString *msg = [NSString stringWithFormat:@"{\"type\":\"join\", \"secretId\":\"%s\", \"userName\":\"%s\", \"roomName\":\"%@\", \"pub\":%s, \"nclr\":%d}\n",
                         g_secretId.c_str(), g_localUserName.c_str(), room, dlg.isPrivate ? "false" : "true", [self getLocalColorInt]];
        if (chatNetwork) chatNetwork->SendMessage([msg UTF8String]);
        if (chatNetwork) chatNetwork->SendMessage("{\"type\":\"get_rooms\"}\n");
        [self appendSystemLogMessage:[NSString stringWithFormat:NSLocalizedString(@"IDS_MOVED_TO", nil), room]];
    }
}

- (void)onShowRenameDialog:(id)sender {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:NSLocalizedString(@"IDS_CHANGE_NAME_TITLE", nil)];
    
    NSView *container = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 200, 60)];
    
    NSTextField *input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 30, 200, 24)];
    [input setStringValue:[NSString stringWithUTF8String:g_localUserName.c_str()]];
    [container addSubview:input];
    
    NSTextField *colorLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 100, 24)];
    colorLabel.stringValue = @"Color:";
    colorLabel.editable = NO;
    colorLabel.bordered = NO;
    colorLabel.drawsBackground = NO;
    [container addSubview:colorLabel];
    
    NSColorWell *colorWell = [[NSColorWell alloc] initWithFrame:NSMakeRect(100, 0, 50, 24)];
    colorWell.color = self.colorWell.color;
    [container addSubview:colorWell];
    
    [alert setAccessoryView:container];
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    if ([alert runModal] == NSAlertFirstButtonReturn) {
        NSString *newName = [input stringValue];
        if (newName.length > 0) {
            g_localUserName = [newName UTF8String];
            self.colorWell.color = colorWell.color;
            int colorInt = [self getLocalColorInt];
            NSString *msg = [NSString stringWithFormat:@"{\"type\":\"rename\", \"secretId\":\"%s\", \"name\":\"%s\", \"nclr\":%d}\n",
                             g_secretId.c_str(), g_localUserName.c_str(), colorInt];
            if (chatNetwork) chatNetwork->SendMessage([msg UTF8String]);
            
            for (auto& u : g_userList) {
                if (u.userId == g_userId) {
                    u.userName = g_localUserName;
                    u.color = colorInt;
                    break;
                }
            }
            [self updateUserListUI];
        }
    }
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    if (tableView == self.userListView) {
        return g_userList.size();
    }
    return 0;
}

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    if (tableView == self.userListView) {
        NSTextField *cell = [tableView makeViewWithIdentifier:@"UserCell" owner:self];
        if (!cell) {
            cell = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, tableColumn.width, 20)];
            cell.identifier = @"UserCell";
            cell.editable = NO;
            cell.bordered = NO;
            cell.drawsBackground = NO;
            cell.font = [NSFont systemFontOfSize:13];
        }
        if (row < g_userList.size()) {
            const UserInfo& u = g_userList[row];
            cell.stringValue = [NSString stringWithUTF8String:u.userName.c_str()];
            
            NSColor *textColor = [NSColor textColor];
            if (u.color != -1 && u.color != 0xFFFFFFFF) {
                CGFloat r = (u.color & 0xFF) / 255.0;
                CGFloat g = ((u.color >> 8) & 0xFF) / 255.0;
                CGFloat b = ((u.color >> 16) & 0xFF) / 255.0;
                textColor = [NSColor colorWithCalibratedRed:r green:g blue:b alpha:1.0];
            }
            cell.textColor = textColor;
        }
        return cell;
    }
    return nil;
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
