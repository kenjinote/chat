#import <AppKit/AppKit.h>
int main() {
    @autoreleasepool {
        NSComboBox *combo = [[NSComboBox alloc] initWithFrame:NSMakeRect(0,0,100,20)];
        combo.bordered = NO;
        NSLog(@"Bordered: %d, Has Button: %d", combo.isBordered, combo.hasVerticalScroller); // NSComboBox has a button, no specific property for it though
    }
    return 0;
}
