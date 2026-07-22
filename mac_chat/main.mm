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

typedef NS_ENUM(NSInteger, Win95ArrowType) {
    Win95ArrowNone = 0,
    Win95ArrowUp,
    Win95ArrowDown
};

@interface Win95Button : NSButton
@property (assign) BOOL isCheckbox;
@property (assign) BOOL isGreenWhenPressed;
@property (assign) BOOL isToggleButton;
@property (assign) BOOL hasMicIcon;
@property (assign) Win95ArrowType arrowType;
@end
@implementation Win95Button
- (BOOL)isFlipped { return YES; }
- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        [self setButtonType:NSButtonTypeMomentaryPushIn];
    }
    return self;
}

- (void)setButtonType:(NSButtonType)type {
    [super setButtonType:type];
    if (type == NSButtonTypeSwitch || type == NSButtonTypeRadio) {
        self.isCheckbox = YES;
        self.isToggleButton = YES;
    } else if (type == NSButtonTypePushOnPushOff) {
        self.isToggleButton = YES;
    } else {
        self.isToggleButton = NO;
    }
}
- (void)drawRect:(NSRect)dirtyRect {
    if (self.isCheckbox) {
        NSRect bounds = self.bounds;
        NSRect boxRect = NSMakeRect(0, (bounds.size.height - 13)/2.0, 13, 13);
        
        // Background
        [[NSColor whiteColor] setFill];
        NSRectFill(boxRect);
        
        // Sunken box
        // Outer Top & Left (Dark Gray)
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(boxRect.origin.x, boxRect.origin.y, boxRect.size.width, 1)); // Top
        NSRectFill(NSMakeRect(boxRect.origin.x, boxRect.origin.y, 1, boxRect.size.height)); // Left
        
        // Inner Top & Left (Black)
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(boxRect.origin.x+1, boxRect.origin.y+1, boxRect.size.width-2, 1));
        NSRectFill(NSMakeRect(boxRect.origin.x+1, boxRect.origin.y+1, 1, boxRect.size.height-2));
        
        // Outer Bottom & Right (White)
        [[NSColor whiteColor] set];
        NSRectFill(NSMakeRect(boxRect.origin.x, boxRect.origin.y + boxRect.size.height - 1, boxRect.size.width, 1));
        NSRectFill(NSMakeRect(boxRect.origin.x + boxRect.size.width - 1, boxRect.origin.y, 1, boxRect.size.height));
        
        // Inner Bottom & Right (Light Gray)
        [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] set];
        NSRectFill(NSMakeRect(boxRect.origin.x+1, boxRect.origin.y + boxRect.size.height - 2, boxRect.size.width-2, 1));
        NSRectFill(NSMakeRect(boxRect.origin.x + boxRect.size.width - 2, boxRect.origin.y+1, 1, boxRect.size.height-2));
        
        if (self.state == NSControlStateValueOn) {
            [[NSColor blackColor] set];
            NSBezierPath *path = [NSBezierPath bezierPath];
            [path moveToPoint:NSMakePoint(boxRect.origin.x + 3, boxRect.origin.y + 6)];
            [path lineToPoint:NSMakePoint(boxRect.origin.x + 5, boxRect.origin.y + 8)];
            [path lineToPoint:NSMakePoint(boxRect.origin.x + 10, boxRect.origin.y + 3)];
            [path setLineWidth:1.5];
            [path stroke];
        }
        
        NSDictionary *attrs = @{NSFontAttributeName: self.font ? self.font : [NSFont systemFontOfSize:12], NSForegroundColorAttributeName: [NSColor blackColor]};
        NSSize size = [self.title sizeWithAttributes:attrs];
        [self.title drawAtPoint:NSMakePoint(18, (bounds.size.height - size.height)/2.0) withAttributes:attrs];
        return;
    }
    
    NSRect bounds = self.bounds;
    
    BOOL pressed = self.isHighlighted;
    if (self.isToggleButton) {
        pressed = pressed || (self.state == NSControlStateValueOn);
    }
    
    if (pressed && self.isGreenWhenPressed) {
        [[NSColor colorWithCalibratedRed:0.0 green:1.0 blue:0.0 alpha:1.0] setFill];
    } else {
        [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
    }
    NSRectFill(bounds);
    
    if (pressed) {
        // Sunken (Pressed)
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1)); // Top
        NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height)); // Left
        
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(1, 1, bounds.size.width-2, 1)); // Inner Top
        NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height-2)); // Inner Left
        
        [[NSColor whiteColor] set];
        NSRectFill(NSMakeRect(0, bounds.size.height-1, bounds.size.width, 1)); // Bottom
        NSRectFill(NSMakeRect(bounds.size.width-1, 0, 1, bounds.size.height)); // Right
    } else {
        // Raised (Unpressed)
        [[NSColor whiteColor] set];
        NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1)); // Top
        NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height)); // Left
        
        [[NSColor colorWithCalibratedRed:0.85 green:0.85 blue:0.85 alpha:1.0] set];
        NSRectFill(NSMakeRect(1, 1, bounds.size.width-2, 1)); // Inner Top
        NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height-2)); // Inner Left
        
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(0, bounds.size.height-1, bounds.size.width, 1)); // Bottom
        NSRectFill(NSMakeRect(bounds.size.width-1, 0, 1, bounds.size.height)); // Right
        
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(1, bounds.size.height-2, bounds.size.width-2, 1)); // Inner Bottom
        NSRectFill(NSMakeRect(bounds.size.width-2, 1, 1, bounds.size.height-2)); // Inner Right
    }
    
    NSMutableParagraphStyle *style = [[NSMutableParagraphStyle alloc] init];
    style.alignment = NSTextAlignmentCenter;
    NSDictionary *attrs = @{NSFontAttributeName: self.font ? self.font : [NSFont systemFontOfSize:12],
                            NSForegroundColorAttributeName: [NSColor blackColor],
                            NSParagraphStyleAttributeName: style};
    NSSize size = [self.title sizeWithAttributes:attrs];
    NSRect titleRect = NSMakeRect(0, (bounds.size.height - size.height) / 2.0, bounds.size.width, size.height);
    if (pressed) { titleRect.origin.x += 1; titleRect.origin.y += 1; }
    if (self.hasMicIcon) {
        // Tightly group mic and text, and exactly center them inside the button
        CGFloat gap = 2.0; // tighter gap for Windows 95 style
        CGFloat micWidth = 10.0;
        CGFloat totalBlockWidth = micWidth + gap + size.width;
        CGFloat startX = round((bounds.size.width - totalBlockWidth) / 2.0);
        
        CGFloat cx = startX + (micWidth / 2.0) - 5.0; // Moved 5px left per user request
        CGFloat cy = round(bounds.size.height / 2.0);
        if (pressed) { cx += 1; cy += 1; }
        
        // Ensure the title is exactly positioned after the mic
        titleRect.origin.x = startX + micWidth + gap;
        titleRect.size.width = size.width; // fix text shifting out of bounds due to center alignment!
        if (pressed) { titleRect.origin.x += 1; }
        
        // Draw Light Gray Inner Head
        [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
        NSRectFill(NSMakeRect(cx - 1, cy - 5, 3, 6));
        
        // Draw Black Outline & Structure
        [[NSColor blackColor] setFill];
        
        // Head Top/Bottom
        NSRectFill(NSMakeRect(cx - 1, cy - 6, 3, 1));
        NSRectFill(NSMakeRect(cx - 1, cy + 1, 3, 1));
        
        // Head Sides
        NSRectFill(NSMakeRect(cx - 2, cy - 5, 1, 6));
        NSRectFill(NSMakeRect(cx + 2, cy - 5, 1, 6));
        
        // U-shape Arms
        NSRectFill(NSMakeRect(cx - 4, cy - 1, 1, 3)); // left arm, y=-1 to 1
        NSRectFill(NSMakeRect(cx + 4, cy - 1, 1, 3)); // right arm, y=-1 to 1
        NSRectFill(NSMakeRect(cx - 3, cy + 2, 1, 1)); // left diagonal
        NSRectFill(NSMakeRect(cx + 3, cy + 2, 1, 1)); // right diagonal
        NSRectFill(NSMakeRect(cx - 2, cy + 3, 5, 1)); // u-shape bottom
        
        // Stem
        NSRectFill(NSMakeRect(cx, cy + 4, 1, 2));
        
        // Base
        NSRectFill(NSMakeRect(cx - 2, cy + 6, 5, 1));
        NSRectFill(NSMakeRect(cx - 3, cy + 7, 7, 1));
    }

    if (self.arrowType != Win95ArrowNone) {
        [[NSColor blackColor] setFill];
        CGFloat cx = round(bounds.size.width / 2.0);
        CGFloat cy = round(bounds.size.height / 2.0);
        if (pressed) { cx += 1; cy += 1; }
        
        if (self.arrowType == Win95ArrowUp) {
            NSRectFill(NSMakeRect(cx, cy - 2, 1, 1));
            NSRectFill(NSMakeRect(cx - 1, cy - 1, 3, 1));
            NSRectFill(NSMakeRect(cx - 2, cy, 5, 1));
            NSRectFill(NSMakeRect(cx - 3, cy + 1, 7, 1));
        } else if (self.arrowType == Win95ArrowDown) {
            NSRectFill(NSMakeRect(cx - 3, cy - 1, 7, 1));
            NSRectFill(NSMakeRect(cx - 2, cy, 5, 1));
            NSRectFill(NSMakeRect(cx - 1, cy + 1, 3, 1));
            NSRectFill(NSMakeRect(cx, cy + 2, 1, 1));
        }
    } else if (self.image) {
        NSSize imgSize = self.image.size;
        NSRect imgRect = NSMakeRect((bounds.size.width - imgSize.width)/2.0, (bounds.size.height - imgSize.height)/2.0, imgSize.width, imgSize.height);
        if (pressed) { imgRect.origin.x += 1; imgRect.origin.y += 1; }
        [self.image drawInRect:imgRect];
    } else {
        [self.title drawInRect:titleRect withAttributes:attrs];
    }
}
@end

@interface Win95TextField : NSTextField
@property (assign) BOOL hasSunkenBorder;
@end
@implementation Win95TextField
- (BOOL)isFlipped { return YES; }
- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) { 
        self.bordered = NO; 
        self.drawsBackground = YES; 
        self.backgroundColor = [NSColor whiteColor];
        self.hasSunkenBorder = YES;
    }
    return self;
}
- (void)drawRect:(NSRect)dirtyRect {
    NSRect bounds = self.bounds;
    [super drawRect:dirtyRect];
    
    if (self.hasSunkenBorder) {
        // Sunken border
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set]; // Dark Gray outer top/left
        NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1));
        NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height));
        
        [[NSColor blackColor] set]; // Black inner top/left
        NSRectFill(NSMakeRect(1, 1, bounds.size.width-2, 1));
        NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height-2));
        
        [[NSColor whiteColor] set]; // White outer bottom/right
        NSRectFill(NSMakeRect(0, bounds.size.height-1, bounds.size.width, 1));
        NSRectFill(NSMakeRect(bounds.size.width-1, 0, 1, bounds.size.height));
        
        [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] set]; // Light Gray inner bottom/right
        NSRectFill(NSMakeRect(1, bounds.size.height-2, bounds.size.width-2, 1));
        NSRectFill(NSMakeRect(bounds.size.width-2, 1, 1, bounds.size.height-2));
    }
}
@end

@interface Win95Scroller : NSScroller
@end

@implementation Win95Scroller
+ (BOOL)isCompatibleWithOverlayScrollers {
    return NO;
}
- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        self.scrollerStyle = NSScrollerStyleLegacy;
    }
    return self;
}
- (void)drawKnobSlotInRect:(NSRect)slotRect highlight:(BOOL)flag {
    // Checkerboard or light gray pattern. 
    // Win95 usually uses a checkered gray for the track, but light gray is acceptable and simpler.
    // Actually, drawing a checkerboard is very Win95!
    [[NSColor colorWithCalibratedWhite:0.88 alpha:1.0] setFill];
    NSRectFill(slotRect);
    
    // Draw a quick checkerboard pattern
    [[NSColor colorWithCalibratedWhite:0.95 alpha:1.0] setFill];
    NSBezierPath *path = [NSBezierPath bezierPath];
    for (int y = slotRect.origin.y; y < slotRect.origin.y + slotRect.size.height; y += 2) {
        for (int x = slotRect.origin.x + (y % 4 == 0 ? 0 : 2); x < slotRect.origin.x + slotRect.size.width; x += 4) {
            [path appendBezierPathWithRect:NSMakeRect(x, y, 2, 2)];
        }
    }
    [path fill];
}
- (void)drawKnob {
    NSRect knobRect = [self rectForPart:NSScrollerKnob];
    
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
    NSRectFill(knobRect);
    
    // Raised 3D effect
    // Top & Left
    [[NSColor whiteColor] set];
    NSRectFill(NSMakeRect(knobRect.origin.x, knobRect.origin.y, knobRect.size.width, 1));
    NSRectFill(NSMakeRect(knobRect.origin.x, knobRect.origin.y, 1, knobRect.size.height));
    
    // Bottom & Right
    [[NSColor blackColor] set];
    NSRectFill(NSMakeRect(knobRect.origin.x, knobRect.origin.y + knobRect.size.height - 1, knobRect.size.width, 1));
    NSRectFill(NSMakeRect(knobRect.origin.x + knobRect.size.width - 1, knobRect.origin.y, 1, knobRect.size.height));
    
    // Inner Top & Left
    [[NSColor colorWithCalibratedRed:0.85 green:0.85 blue:0.85 alpha:1.0] set];
    NSRectFill(NSMakeRect(knobRect.origin.x + 1, knobRect.origin.y + 1, knobRect.size.width - 2, 1));
    NSRectFill(NSMakeRect(knobRect.origin.x + 1, knobRect.origin.y + 1, 1, knobRect.size.height - 2));
    
    // Inner Bottom & Right
    [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
    NSRectFill(NSMakeRect(knobRect.origin.x + 1, knobRect.origin.y + knobRect.size.height - 2, knobRect.size.width - 2, 1));
    NSRectFill(NSMakeRect(knobRect.origin.x + knobRect.size.width - 2, knobRect.origin.y + 1, 1, knobRect.size.height - 2));
}

// Deprecated in recent macOS, but try drawing arrows if the OS calls it
- (void)drawArrow:(NSScrollerArrow)whichArrow highlight:(BOOL)flag {
    NSRect arrowRect = [self rectForPart:(whichArrow == NSScrollerDecrementArrow ? NSScrollerDecrementLine : NSScrollerIncrementLine)];
    if (NSIsEmptyRect(arrowRect)) return;
    
    if (flag) {
        [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
        NSRectFill(arrowRect);
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(arrowRect.origin.x, arrowRect.origin.y, arrowRect.size.width, 1));
        NSRectFill(NSMakeRect(arrowRect.origin.x, arrowRect.origin.y, 1, arrowRect.size.height));
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(arrowRect.origin.x+1, arrowRect.origin.y+1, arrowRect.size.width-2, 1));
        NSRectFill(NSMakeRect(arrowRect.origin.x+1, arrowRect.origin.y+1, 1, arrowRect.size.height-2));
    } else {
        [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
        NSRectFill(arrowRect);
        [[NSColor whiteColor] set];
        NSRectFill(NSMakeRect(arrowRect.origin.x, arrowRect.origin.y, arrowRect.size.width, 1));
        NSRectFill(NSMakeRect(arrowRect.origin.x, arrowRect.origin.y, 1, arrowRect.size.height));
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(arrowRect.origin.x, arrowRect.origin.y + arrowRect.size.height - 1, arrowRect.size.width, 1));
        NSRectFill(NSMakeRect(arrowRect.origin.x + arrowRect.size.width - 1, arrowRect.origin.y, 1, arrowRect.size.height));
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(arrowRect.origin.x+1, arrowRect.origin.y + arrowRect.size.height - 2, arrowRect.size.width-2, 1));
        NSRectFill(NSMakeRect(arrowRect.origin.x + arrowRect.size.width - 2, arrowRect.origin.y+1, 1, arrowRect.size.height-2));
    }
    
    [[NSColor blackColor] setFill];
    NSBezierPath *path = [NSBezierPath bezierPath];
    CGFloat cx = NSMidX(arrowRect);
    CGFloat cy = NSMidY(arrowRect);
    if (flag) { cx += 1; cy += 1; }
    
    if (self.bounds.size.width > self.bounds.size.height) { // Horizontal
        if (whichArrow == NSScrollerDecrementArrow) {
            [path moveToPoint:NSMakePoint(cx - 2, cy)];
            [path lineToPoint:NSMakePoint(cx + 2, cy - 3)];
            [path lineToPoint:NSMakePoint(cx + 2, cy + 3)];
        } else {
            [path moveToPoint:NSMakePoint(cx + 2, cy)];
            [path lineToPoint:NSMakePoint(cx - 2, cy - 3)];
            [path lineToPoint:NSMakePoint(cx - 2, cy + 3)];
        }
    } else { // Vertical
        if (whichArrow == NSScrollerDecrementArrow) { // Up
            [path moveToPoint:NSMakePoint(cx, cy - 2)];
            [path lineToPoint:NSMakePoint(cx - 3, cy + 2)];
            [path lineToPoint:NSMakePoint(cx + 3, cy + 2)];
        } else { // Down
            [path moveToPoint:NSMakePoint(cx, cy + 2)];
            [path lineToPoint:NSMakePoint(cx - 3, cy - 2)];
            [path lineToPoint:NSMakePoint(cx + 3, cy - 2)];
        }
    }
    [path fill];
}
@end

@interface Win95ComboBox : NSComboBox
@end
@implementation Win95ComboBox
- (BOOL)isFlipped { return YES; }

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        self.bordered = NO;
        self.backgroundColor = [NSColor whiteColor];
        self.drawsBackground = YES;
        self.focusRingType = NSFocusRingTypeNone;
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    NSRect bounds = self.bounds;
    
    // 1. Draw Sunken Border around the text field part
    [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
    NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1)); // Top
    NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height)); // Left
    
    [[NSColor blackColor] set];
    NSRectFill(NSMakeRect(1, 1, bounds.size.width-2, 1));
    NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height-2));
    
    [[NSColor whiteColor] set];
    NSRectFill(NSMakeRect(0, bounds.size.height - 1, bounds.size.width, 1));
    NSRectFill(NSMakeRect(bounds.size.width - 1, 0, 1, bounds.size.height));
    
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] set];
    NSRectFill(NSMakeRect(1, bounds.size.height - 2, bounds.size.width - 2, 1));
    NSRectFill(NSMakeRect(bounds.size.width - 2, 1, 1, bounds.size.height - 2));
    
    // 2. Draw the Windows 95 button over the right side
    CGFloat btnWidth = 16.0;
    NSRect btnRect = NSMakeRect(bounds.size.width - btnWidth - 2, 2, btnWidth, bounds.size.height - 4);
    
    // Erase the native macOS blue arrow that bleeds out to the left of our button
    [[NSColor whiteColor] setFill];
    NSRect eraseRect = NSMakeRect(btnRect.origin.x - 6, 2, 6, bounds.size.height - 4);
    NSRectFill(eraseRect);
    
    // We cover whatever was drawn under this rectangle
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
    NSRectFill(btnRect);
    
    BOOL pressed = [self.cell isHighlighted];
    
    // Raised 3D Button Border
    if (pressed) {
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(btnRect.origin.x, btnRect.origin.y, btnRect.size.width, 1));
        NSRectFill(NSMakeRect(btnRect.origin.x, btnRect.origin.y, 1, btnRect.size.height));
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(btnRect.origin.x+1, btnRect.origin.y+1, btnRect.size.width-2, 1));
        NSRectFill(NSMakeRect(btnRect.origin.x+1, btnRect.origin.y+1, 1, btnRect.size.height-2));
    } else {
        [[NSColor whiteColor] set];
        NSRectFill(NSMakeRect(btnRect.origin.x, btnRect.origin.y, btnRect.size.width, 1));
        NSRectFill(NSMakeRect(btnRect.origin.x, btnRect.origin.y, 1, btnRect.size.height));
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(btnRect.origin.x, btnRect.origin.y + btnRect.size.height - 1, btnRect.size.width, 1));
        NSRectFill(NSMakeRect(btnRect.origin.x + btnRect.size.width - 1, btnRect.origin.y, 1, btnRect.size.height));
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(btnRect.origin.x+1, btnRect.origin.y + btnRect.size.height - 2, btnRect.size.width-2, 1));
        NSRectFill(NSMakeRect(btnRect.origin.x + btnRect.size.width - 2, btnRect.origin.y+1, 1, btnRect.size.height-2));
    }
    
    // Pixel-perfect Down Arrow (centered in the button)
    CGFloat cx = round(NSMidX(btnRect));
    CGFloat cy = round(NSMidY(btnRect));
    if (pressed) { cx += 1; cy += 1; }
    
    [[NSColor blackColor] setFill];
    NSRectFill(NSMakeRect(cx - 3, cy - 1, 7, 1));
    NSRectFill(NSMakeRect(cx - 2, cy, 5, 1));
    NSRectFill(NSMakeRect(cx - 1, cy + 1, 3, 1));
    NSRectFill(NSMakeRect(cx, cy + 2, 1, 1));
}
@end

@interface Win95ColorWell : NSControl
@property (nonatomic, strong) NSColor *color;
@property (nonatomic, assign) BOOL isPressed;
@property (nonatomic, strong) NSColor *originalColor;
@property (nonatomic, assign) BOOL confirmedColor;
@end

@implementation Win95ColorWell

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        _color = [NSColor blackColor]; // Default color
    }
    return self;
}

- (void)setColor:(NSColor *)color {
    if (_color != color) {
        _color = color;
        [self setNeedsDisplay:YES];
    }
}

- (BOOL)isFlipped { return YES; }

- (void)onColorPanelOK:(id)sender {
    self.confirmedColor = YES;
    [[NSColorPanel sharedColorPanel] close];
}

- (void)onColorPanelCancel:(id)sender {
    self.confirmedColor = NO;
    [[NSColorPanel sharedColorPanel] close];
}

- (void)mouseDown:(NSEvent *)event {
    self.isPressed = YES;
    [self setNeedsDisplay:YES];
    
    self.originalColor = self.color;
    self.confirmedColor = NO;
    
    NSColorPanel *panel = [NSColorPanel sharedColorPanel];
    panel.color = self.color;
    panel.showsAlpha = NO;
    
    // Setup OK / Cancel accessory view
    NSView *accView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, panel.frame.size.width, 40)];
    Win95Button *okBtn = [[Win95Button alloc] initWithFrame:NSMakeRect(accView.bounds.size.width - 160, 8, 70, 24)];
    okBtn.title = @"OK";
    okBtn.target = self;
    okBtn.action = @selector(onColorPanelOK:);
    [accView addSubview:okBtn];
    
    Win95Button *cancelBtn = [[Win95Button alloc] initWithFrame:NSMakeRect(accView.bounds.size.width - 80, 8, 70, 24)];
    cancelBtn.title = @"キャンセル";
    cancelBtn.target = self;
    cancelBtn.action = @selector(onColorPanelCancel:);
    [accView addSubview:cancelBtn];
    
    panel.accessoryView = accView;
    
    // Center it on the main window
    NSRect windowFrame = self.window.frame;
    NSRect panelFrame = panel.frame;
    panelFrame.origin.x = windowFrame.origin.x + (windowFrame.size.width - panelFrame.size.width) / 2.0;
    panelFrame.origin.y = windowFrame.origin.y + (windowFrame.size.height - panelFrame.size.height) / 2.0;
    [panel setFrame:panelFrame display:YES];
    
    id observer = [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowWillCloseNotification object:panel queue:nil usingBlock:^(NSNotification * _Nonnull note) {
        [NSApp stopModal];
    }];
    
    [NSApp runModalForWindow:panel];
    
    [[NSNotificationCenter defaultCenter] removeObserver:observer];
    panel.accessoryView = nil; // Clean up
    
    if (self.confirmedColor) {
        self.color = panel.color;
        if (self.action) {
            [NSApp sendAction:self.action to:self.target from:self];
        }
    } else {
        self.color = self.originalColor;
    }
    
    self.isPressed = NO;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    NSRect bounds = self.bounds;
    BOOL pressed = self.isPressed;
    
    // 1. Draw Button Background
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
    NSRectFill(bounds);
    
    // 2. Draw 3D Button Bevel
    if (pressed) {
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1));
        NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height));
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(1, 1, bounds.size.width-2, 1));
        NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height-2));
    } else {
        [[NSColor whiteColor] set];
        NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1));
        NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height));
        [[NSColor blackColor] set];
        NSRectFill(NSMakeRect(0, bounds.size.height - 1, bounds.size.width, 1));
        NSRectFill(NSMakeRect(bounds.size.width - 1, 0, 1, bounds.size.height));
        [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set];
        NSRectFill(NSMakeRect(1, bounds.size.height - 2, bounds.size.width-2, 1));
        NSRectFill(NSMakeRect(bounds.size.width - 2, 1, 1, bounds.size.height-2));
    }
    
    // 3. Draw Square Color Preview
    CGFloat squareSize = 12.0;
    NSRect colorRect = NSMakeRect(round((bounds.size.width - squareSize) / 2.0), round((bounds.size.height - squareSize) / 2.0), squareSize, squareSize);
    
    if (pressed) {
        colorRect.origin.x += 1;
        colorRect.origin.y += 1;
    }
    
    // Black border around the color square
    [[NSColor blackColor] setFill];
    NSRect borderRect = NSMakeRect(colorRect.origin.x - 1, colorRect.origin.y - 1, colorRect.size.width + 2, colorRect.size.height + 2);
    NSRectFill(borderRect);
    
    // The actual selected color
    [self.color setFill];
    NSRectFill(colorRect);
}
@end

@interface Win95EdgeView : NSView
@end
@implementation Win95EdgeView
- (BOOL)isFlipped { return YES; }
- (void)drawRect:(NSRect)dirtyRect {
    NSRect bounds = self.bounds;
    [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] setFill];
    NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1));
    NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height));
    [[NSColor blackColor] setFill];
    NSRectFill(NSMakeRect(1, 1, bounds.size.width - 2, 1));
    NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height - 2));
    [[NSColor whiteColor] setFill];
    NSRectFill(NSMakeRect(0, bounds.size.height - 1, bounds.size.width, 1));
    NSRectFill(NSMakeRect(bounds.size.width - 1, 0, 1, bounds.size.height));
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] setFill];
    NSRectFill(NSMakeRect(1, bounds.size.height - 2, bounds.size.width - 2, 1));
    NSRectFill(NSMakeRect(bounds.size.width - 2, 1, 1, bounds.size.height - 2));
}
@end

@interface Win95HeaderLabel : NSTextField
@end
@implementation Win95HeaderLabel
- (BOOL)isFlipped { return YES; }
- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        self.bordered = NO;
        self.drawsBackground = YES;
        self.backgroundColor = [NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0];
    }
    return self;
}
- (void)drawRect:(NSRect)dirtyRect {
    NSRect bounds = self.bounds;
    [self.backgroundColor setFill];
    NSRectFill(bounds);
    
    // Half-raised border (1px instead of 2px)
    [[NSColor whiteColor] setFill]; // Top/Left Outer
    NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1));
    NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height));
    
    [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] setFill]; // Dark Gray Bottom/Right Outer
    NSRectFill(NSMakeRect(0, bounds.size.height - 1, bounds.size.width, 1));
    NSRectFill(NSMakeRect(bounds.size.width - 1, 0, 1, bounds.size.height));
    
    [super drawRect:dirtyRect];
}
@end

@interface Win95ScrollView : NSScrollView
@property (strong) Win95Button *vUpBtn;
@property (strong) Win95Button *vDownBtn;
@property (strong) NSTimer *scrollTimer;
@end
@implementation Win95ScrollView
- (BOOL)isFlipped { return YES; }
- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) { 
        self.borderType = NSNoBorder; 
        self.drawsBackground = YES; 
        self.backgroundColor = [NSColor whiteColor]; 
        
        Win95Scroller *vScroller = [[Win95Scroller alloc] init];
        self.verticalScroller = vScroller;
        
        self.vUpBtn = [[Win95Button alloc] initWithFrame:NSZeroRect];
        self.vUpBtn.title = @"";
        self.vUpBtn.arrowType = Win95ArrowUp;
        self.vUpBtn.target = self;
        self.vUpBtn.action = @selector(doScrollUp);
        [self addSubview:self.vUpBtn];
        
        self.vDownBtn = [[Win95Button alloc] initWithFrame:NSZeroRect];
        self.vDownBtn.title = @"";
        self.vDownBtn.arrowType = Win95ArrowDown;
        self.vDownBtn.target = self;
        self.vDownBtn.action = @selector(doScrollDown);
        [self addSubview:self.vDownBtn];
    }
    return self;
}

- (void)tile {
    [super tile];
    if (self.hasVerticalScroller && self.verticalScroller) {
        NSRect vsFrame = self.verticalScroller.frame;
        CGFloat btnHeight = vsFrame.size.width; // square button
        
        self.vUpBtn.frame = NSMakeRect(vsFrame.origin.x, vsFrame.origin.y, vsFrame.size.width, btnHeight);
        self.vDownBtn.frame = NSMakeRect(vsFrame.origin.x, vsFrame.origin.y + vsFrame.size.height - btnHeight, vsFrame.size.width, btnHeight);
        
        vsFrame.origin.y += btnHeight;
        vsFrame.size.height -= btnHeight * 2;
        self.verticalScroller.frame = vsFrame;
        
        self.vUpBtn.hidden = NO;
        self.vDownBtn.hidden = NO;
    } else {
        self.vUpBtn.hidden = YES;
        self.vDownBtn.hidden = YES;
    }
}

- (void)doScrollUp {
    NSPoint pt = self.contentView.bounds.origin;
    pt.y -= 20;
    if (pt.y < 0) pt.y = 0;
    [self.contentView scrollToPoint:pt];
    [self reflectScrolledClipView:self.contentView];
}

- (void)doScrollDown {
    NSPoint pt = self.contentView.bounds.origin;
    pt.y += 20;
    NSRect docRect = [self.documentView frame];
    NSRect clipRect = [self.contentView bounds];
    if (pt.y > docRect.size.height - clipRect.size.height) pt.y = docRect.size.height - clipRect.size.height;
    if (pt.y < 0) pt.y = 0;
    [self.contentView scrollToPoint:pt];
    [self reflectScrolledClipView:self.contentView];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    NSRect bounds = self.bounds;
    // Sunken border
    [[NSColor colorWithCalibratedRed:0.5 green:0.5 blue:0.5 alpha:1.0] set]; // Dark Gray outer top/left
    NSRectFill(NSMakeRect(0, 0, bounds.size.width, 1));
    NSRectFill(NSMakeRect(0, 0, 1, bounds.size.height));
    
    [[NSColor blackColor] set]; // Black inner top/left
    NSRectFill(NSMakeRect(1, 1, bounds.size.width-2, 1));
    NSRectFill(NSMakeRect(1, 1, 1, bounds.size.height-2));
    
    [[NSColor whiteColor] set]; // White outer bottom/right
    NSRectFill(NSMakeRect(0, bounds.size.height-1, bounds.size.width, 1));
    NSRectFill(NSMakeRect(bounds.size.width-1, 0, 1, bounds.size.height));
    
    [[NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0] set]; // Light Gray inner bottom/right
    NSRectFill(NSMakeRect(1, bounds.size.height-2, bounds.size.width-2, 1));
    NSRectFill(NSMakeRect(bounds.size.width-2, 1, 1, bounds.size.height-2));
}
@end


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
        
        self.nameLabel = [[Win95TextField alloc] initWithFrame:NSMakeRect(28, 2, frameRect.size.width - 30, 24)];
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
    win.backgroundColor = [NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0];
    self = [super initWithWindow:win];
    if (self) {
        self.allRooms = [NSMutableArray array];
        self.filteredRooms = [NSMutableArray array];
        NSView *content = win.contentView;
        
        self.selectedRoom = currentRoom ? currentRoom : NSLocalizedString(@"IDS_LOBBY", nil);
        self.roomField = [[Win95TextField alloc] initWithFrame:NSMakeRect(15, 230, 270, 24)];
        self.roomField.stringValue = @"";
        self.roomField.placeholderString = NSLocalizedString(@"IDS_MOVE_ROOM_TITLE", nil);
        self.roomField.delegate = self;
        [content addSubview:self.roomField];
        
        NSScrollView *scroll = [[Win95ScrollView alloc] initWithFrame:NSMakeRect(15, 50, 270, 170)];
        scroll.hasVerticalScroller = YES;
        scroll.autohidesScrollers = YES;
        
        self.roomTableView = [[NSTableView alloc] initWithFrame:scroll.bounds];
        if (@available(macOS 11.0, *)) { self.roomTableView.style = NSTableViewStylePlain; }
        self.roomTableView.focusRingType = NSFocusRingTypeNone;
        self.roomTableView.intercellSpacing = NSMakeSize(0, 0);
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
        
        self.privateCheckbox = [[Win95Button alloc] initWithFrame:NSMakeRect(15, 15, 90, 25)];
        [self.privateCheckbox setButtonType:NSButtonTypeSwitch];
        self.privateCheckbox.title = NSLocalizedString(@"IDS_PRIVATE", nil);
        self.privateCheckbox.state = NSControlStateValueOff;
        [content addSubview:self.privateCheckbox];
        
        self.okButton = [[Win95Button alloc] initWithFrame:NSMakeRect(135, 12, 70, 30)];
        self.okButton.title = @"OK";
        self.okButton.bezelStyle = NSBezelStyleRounded;
        self.okButton.target = self;
        self.okButton.action = @selector(onOK:);
        [content addSubview:self.okButton];
        
        self.cancelButton = [[Win95Button alloc] initWithFrame:NSMakeRect(210, 12, 75, 30)];
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
    }
    cell.editable = NO;
    cell.bordered = NO;
    cell.drawsBackground = NO;
    cell.font = [NSFont systemFontOfSize:13];
    cell.textColor = [NSColor labelColor];
    
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
@property (strong) Win95EdgeView *logEdge;
@property (strong) NSScrollView *logScroll;
@property (strong) NSTextView *logTextView;
@property (strong) Win95EdgeView *inputEdge;
@property (strong) NSScrollView *inputScroll;
@property (strong) NSTextView *inputTextView;
@property (strong) Win95HeaderLabel *userListHeader;
@property (strong) Win95EdgeView *userEdge;
@property (strong) NSScrollView *userScroll;
@property (strong) NSTextView *userListView;
@property (strong) NSButton *sendButton;
@property (strong) NSButton *voiceButton;

@property (strong) NSButton *btnB;
@property (strong) NSButton *btnI;
@property (strong) NSButton *btnU;
@property (strong) NSComboBox *sizeCombo;
@property (strong) Win95ColorWell *colorWell;
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
    win.backgroundColor = [NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0];
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

- (void)applicationWillFinishLaunching:(NSNotification *)notification {
    NSMenu *mainMenu = [[NSMenu alloc] initWithTitle:@"MainMenu"];
    
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu *appMenu = [[NSMenu alloc] initWithTitle:@"RetroChat"];
    
    NSMenuItem *quitItem = [[NSMenuItem alloc] initWithTitle:@"終了" action:@selector(terminate:) keyEquivalent:@"q"];
    [appMenu addItem:quitItem];
    
    [appMenuItem setSubmenu:appMenu];
    [mainMenu addItem:appMenuItem];
    
    [NSApp setMainMenu:mainMenu];
}

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
    self.window.backgroundColor = [NSColor colorWithCalibratedRed:0.75 green:0.75 blue:0.75 alpha:1.0];
    [self.window setTitle:@"RetroChat"];
    [self.window center];
    self.window.delegate = self;
    
    self.logEdge = [[Win95EdgeView alloc] initWithFrame:NSZeroRect];
    [self.window.contentView addSubview:self.logEdge];
    self.logScroll = [[Win95ScrollView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    [self.logEdge addSubview:self.logScroll];
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
    
    
    self.userListHeader = [[Win95HeaderLabel alloc] initWithFrame:NSZeroRect];
    self.userListHeader.editable = NO;
    self.userListHeader.bordered = NO;
    self.userListHeader.drawsBackground = NO;
    self.userListHeader.alignment = NSTextAlignmentCenter;
    [self.window.contentView addSubview:self.userListHeader];
    
    self.userEdge = [[Win95EdgeView alloc] initWithFrame:NSZeroRect];
    [self.window.contentView addSubview:self.userEdge];
    self.userScroll = [[Win95ScrollView alloc] initWithFrame:NSZeroRect];
    [self.userEdge addSubview:self.userScroll];
    self.userScroll.hasVerticalScroller = YES;
    self.userListView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, self.userScroll.bounds.size.width, self.userScroll.bounds.size.height)];
    self.userListView.editable = NO;
    self.userListView.selectable = NO;
    self.userListView.richText = YES;
    self.userListView.autoresizingMask = NSViewWidthSizable;
    self.userListView.menu = [[NSMenu alloc] initWithTitle:@"Context"];
    self.userListView.menu.delegate = self;
    self.userScroll.documentView = self.userListView;
    
    
    self.btnB = [[Win95Button alloc] initWithFrame:NSZeroRect];
    self.btnB.bezelStyle = NSBezelStyleRegularSquare;
    self.btnB.title = @"B";
    self.btnB.target = self;
    self.btnB.action = @selector(onFormatChanged:);
    [self.window.contentView addSubview:self.btnB];
    
    self.btnI = [[Win95Button alloc] initWithFrame:NSZeroRect];
    self.btnI.bezelStyle = NSBezelStyleRegularSquare;
    self.btnI.title = @"I";
    self.btnI.target = self;
    self.btnI.action = @selector(onFormatChanged:);
    [self.window.contentView addSubview:self.btnI];
    
    self.btnU = [[Win95Button alloc] initWithFrame:NSZeroRect];
    self.btnU.bezelStyle = NSBezelStyleRegularSquare;
    self.btnU.title = @"U";
    self.btnU.target = self;
    self.btnU.action = @selector(onFormatChanged:);
    [self.window.contentView addSubview:self.btnU];
    
    self.sizeCombo = [[Win95ComboBox alloc] initWithFrame:NSZeroRect];
    [self.sizeCombo addItemsWithObjectValues:@[@"6", @"8", @"9", @"10", @"11", @"12", @"14", @"16", @"18", @"20", @"22", @"24", @"26", @"28", @"32"]];
    [self.sizeCombo selectItemAtIndex:5];
    self.sizeCombo.target = self;
    self.sizeCombo.action = @selector(onFormatChanged:);
    self.sizeCombo.delegate = self;
    [self.window.contentView addSubview:self.sizeCombo];
    
    self.colorWell = [[Win95ColorWell alloc] initWithFrame:NSZeroRect];
    self.colorWell.target = self;
    self.colorWell.action = @selector(onFormatChanged:);
    [self.window.contentView addSubview:self.colorWell];
    
    self.currentRoomName = NSLocalizedString(@"IDS_LOBBY", nil);
    [self updateWindowTitle];
    self.moveRoomBtn = [[Win95Button alloc] initWithFrame:NSZeroRect];
    self.moveRoomBtn.title = NSLocalizedString(@"IDS_MOVE_ROOM_BTN", nil);
    self.moveRoomBtn.target = self;
    self.moveRoomBtn.action = @selector(onMoveRoomClicked:);
    [self.window.contentView addSubview:self.moveRoomBtn];
    
    self.voiceButton = [[Win95Button alloc] initWithFrame:NSZeroRect];
    [self.voiceButton setButtonType:NSButtonTypePushOnPushOff];
    ((Win95Button*)self.voiceButton).isGreenWhenPressed = YES;
    ((Win95Button*)self.voiceButton).hasMicIcon = YES;
    self.voiceButton.title = NSLocalizedString(@"IDS_TALK", nil);
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
    
    self.maintenanceView = [[Win95TextField alloc] initWithFrame:NSZeroRect];
    self.maintenanceView.hidden = YES;
    self.maintenanceView.editable = NO;
    self.maintenanceView.bordered = NO;
    self.maintenanceView.drawsBackground = NO;
    ((Win95TextField*)self.maintenanceView).hasSunkenBorder = NO;
    [self.window.contentView addSubview:self.maintenanceView];
    
    self.inputEdge = [[Win95EdgeView alloc] initWithFrame:NSZeroRect];
    [self.window.contentView addSubview:self.inputEdge];
    self.inputScroll = [[Win95ScrollView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    [self.inputEdge addSubview:self.inputScroll];
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
    
    
    self.sendButton = [[Win95Button alloc] initWithFrame:NSZeroRect];
    self.sendButton.title = NSLocalizedString(@"IDS_SEND", nil);
    self.sendButton.target = self;
    self.sendButton.action = @selector(onSendClicked:);
    [self.window.contentView addSubview:self.sendButton];
    
    [self layoutControls];
    [self updateUserListUI]; // Ensure the header displays '0' instead of blank at startup
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
    NSRect logFrame = NSMakeRect(margin, topPanelBottomY, logWidth, topHeight);
    self.logEdge.frame = logFrame;
    self.logScroll.frame = NSMakeRect(2, 2, logFrame.size.width - 4, logFrame.size.height - 4);
    
    // 2. Right Panel (User Header, User List, Move Room Button)
    CGFloat rightPanelX = margin * 2 + logWidth;
    
    self.userListHeader.frame = NSMakeRect(rightPanelX, topPanelBottomY + topHeight - headerHeight, listWidth, headerHeight);
    
    self.moveRoomBtn.frame = NSMakeRect(rightPanelX, topPanelBottomY, listWidth, roomBtnHeight);
    
    CGFloat userListY = topPanelBottomY + roomBtnHeight + margin;
    CGFloat userListHeight = topHeight - roomBtnHeight - headerHeight - margin;
    if (userListHeight < 30) userListHeight = 30;
    NSRect userFrame = NSMakeRect(rightPanelX, userListY, listWidth, userListHeight);
    self.userEdge.frame = userFrame;
    self.userScroll.frame = NSMakeRect(2, 2, userFrame.size.width - 4, userFrame.size.height - 4);
    
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
    
    self.voiceButton.frame = NSMakeRect(currentX, toolY, 65, toolHeight);
    currentX += 70;
    
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
    
    NSRect inputFrame = NSMakeRect(margin, bottomY, inputWidth, inputHeight);
    self.inputEdge.frame = inputFrame;
    self.inputScroll.frame = NSMakeRect(2, 2, inputFrame.size.width - 4, inputFrame.size.height - 4);
    self.sendButton.frame = NSMakeRect(margin * 2 + inputWidth, bottomY, btnWidth, inputHeight);
}

- (void)windowDidResize:(NSNotification *)notification {
    [self layoutControls];
}

- (void)updateWindowTitle {
    NSString *title = [NSString stringWithFormat:@"RetroChat - [%@]", self.currentRoomName];
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
        self.voiceButton.title = NSLocalizedString(@"IDS_TALK", nil);
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

- (NSInteger)clickedRowInUserList {
    NSPoint pt = [self.window mouseLocationOutsideOfEventStream];
    NSPoint localPt = [self.userListView convertPoint:pt fromView:nil];
    NSLayoutManager *layoutManager = self.userListView.layoutManager;
    NSTextContainer *textContainer = self.userListView.textContainer;
    NSUInteger glyphIndex = [layoutManager glyphIndexForPoint:localPt inTextContainer:textContainer fractionOfDistanceThroughGlyph:NULL];
    NSUInteger charIndex = [layoutManager characterIndexForGlyphAtIndex:glyphIndex];
    NSRect usedRect = [layoutManager usedRectForTextContainer:textContainer];
    if (localPt.y > NSMaxY(usedRect)) {
        return -1;
    }
    NSString *text = self.userListView.string;
    NSRange range = NSMakeRange(0, charIndex);
    NSUInteger row = [[text substringWithRange:range] componentsSeparatedByString:@"\n"].count - 1;
    return row;
}

- (void)menuNeedsUpdate:(NSMenu *)menu {
    [menu removeAllItems];
    NSInteger row = [self clickedRowInUserList];
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
                        if ([owner isEqualToString:@"RetroChat"] || [owner isEqualToString:@"Terminal"]) continue;
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
    NSInteger row = [self clickedRowInUserList];
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
    NSInteger row = [self clickedRowInUserList];
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
    NSMutableAttributedString *attrStr = [[NSMutableAttributedString alloc] init];
    for (size_t i = 0; i < g_userList.size(); i++) {
        const UserInfo& u = g_userList[i];
        NSString *name = [NSString stringWithUTF8String:u.userName.c_str()];
        NSColor *textColor = [NSColor textColor];
        if (u.color != -1 && u.color != 0xFFFFFFFF) {
            CGFloat r = (u.color & 0xFF) / 255.0;
            CGFloat g = ((u.color >> 8) & 0xFF) / 255.0;
            CGFloat b = ((u.color >> 16) & 0xFF) / 255.0;
            textColor = [NSColor colorWithCalibratedRed:r green:g blue:b alpha:1.0];
        }
        NSAttributedString *line = [[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@"%@\n", name] attributes:@{ NSForegroundColorAttributeName: textColor, NSFontAttributeName: [NSFont systemFontOfSize:13] }];
        [attrStr appendAttributedString:line];
    }
    [[self.userListView textStorage] setAttributedString:attrStr];
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
    
    NSTextField *input = [[Win95TextField alloc] initWithFrame:NSMakeRect(0, 30, 200, 24)];
    [input setStringValue:[NSString stringWithUTF8String:g_localUserName.c_str()]];
    [container addSubview:input];
    
    NSTextField *colorLabel = [[Win95TextField alloc] initWithFrame:NSMakeRect(0, 0, 100, 24)];
    colorLabel.stringValue = @"Color:";
    colorLabel.editable = NO;
    colorLabel.bordered = NO;
    colorLabel.drawsBackground = NO;
    [container addSubview:colorLabel];
    
    Win95ColorWell *colorWell = [[Win95ColorWell alloc] initWithFrame:NSMakeRect(100, 0, 50, 24)];
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
    return 0;
}

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
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
