#import <AppKit/AppKit.h>
int main() {
    @autoreleasepool {
        NSButton *b = [[NSButton alloc] init];
        NSLog(@"Flipped: %d", b.isFlipped);
    }
    return 0;
}
