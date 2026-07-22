#import <AppKit/AppKit.h>
int main() {
    @autoreleasepool {
        NSColorPanel *panel = [NSColorPanel sharedColorPanel];
        NSView *acc = [[NSView alloc] initWithFrame:NSMakeRect(0,0,200,40)];
        NSButton *ok = [NSButton buttonWithTitle:@"OK" target:nil action:nil];
        ok.frame = NSMakeRect(100, 10, 80, 24);
        [acc addSubview:ok];
        panel.accessoryView = acc;
        NSLog(@"Set accessory view: %@", panel.accessoryView);
    }
    return 0;
}
