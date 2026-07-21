#import <Cocoa/Cocoa.h>
int main() {
    NSScrollView *scroll = [[NSScrollView alloc] initWithFrame:NSZeroRect];
    NSTextView *tv = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, 1000, 1000)];
    tv.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    scroll.documentView = tv;
    NSLog(@"Before layout: %@", NSStringFromRect(tv.frame));
    scroll.frame = NSMakeRect(0, 0, 500, 500);
    NSLog(@"After layout: %@", NSStringFromRect(tv.frame));
    return 0;
}
