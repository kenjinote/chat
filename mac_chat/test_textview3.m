#import <Cocoa/Cocoa.h>
int main() {
    NSScrollView *scroll = [[NSScrollView alloc] initWithFrame:NSZeroRect];
    scroll.hasVerticalScroller = YES;
    
    NSTextView *tv = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    tv.minSize = NSMakeSize(0.0, 0.0);
    tv.maxSize = NSMakeSize(FLT_MAX, FLT_MAX);
    tv.verticallyResizable = YES;
    tv.horizontallyResizable = NO;
    tv.autoresizingMask = NSViewWidthSizable;
    [tv.textContainer setContainerSize:NSMakeSize(100, FLT_MAX)];
    [tv.textContainer setWidthTracksTextView:YES];
    
    scroll.documentView = tv;
    
    scroll.frame = NSMakeRect(0, 0, 500, 500);
    NSLog(@"After layout: %@", NSStringFromRect(tv.frame));
    return 0;
}
