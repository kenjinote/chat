#import <Cocoa/Cocoa.h>
int main() {
    NSScrollView *scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    scroll.hasVerticalScroller = YES;
    
    NSTextView *tv = [[NSTextView alloc] initWithFrame:scroll.bounds];
    tv.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    scroll.documentView = tv;
    
    scroll.frame = NSMakeRect(0, 0, 500, 500);
    NSLog(@"After layout: %@", NSStringFromRect(tv.frame));
    return 0;
}
