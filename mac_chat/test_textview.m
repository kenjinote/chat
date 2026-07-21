#import <Cocoa/Cocoa.h>
int main() {
    NSScrollView *scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    NSTextView *tv = [[NSTextView alloc] initWithFrame:scroll.bounds];
    scroll.documentView = tv;
    NSLog(@"Before layout: %@", NSStringFromRect(tv.frame));
    scroll.frame = NSMakeRect(0, 0, 500, 500);
    NSLog(@"After layout: %@", NSStringFromRect(tv.frame));
    
    tv.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    scroll.frame = NSMakeRect(0, 0, 600, 600);
    NSLog(@"After layout 2: %@", NSStringFromRect(tv.frame));
    
    return 0;
}
