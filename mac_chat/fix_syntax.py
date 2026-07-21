import re

with open("main.mm", "r") as f:
    text = f.read()

# Fix 1: Duplicate VideoWindowController and mangled SnappableWindow
text = text.replace("""@interface VideoWindowController : NSWindowController <NSWindowDelegate>
@property (strong) AVSampleBufferDisplayLayer *videoLayer;
@property (strong) NSImageView *jpegView;
    [super setFrame:snappedFrame display:flag];
    self.isProgrammaticSet = NO;
}
@end""", """        }
    }
    
    if (cornerSnapped) {
        snappedFrame.origin.x -= minDx;
        snappedFrame.origin.y -= minDy;
    }
    
    [super setFrame:snappedFrame display:flag];
    self.isProgrammaticSet = NO;
}
""")

# Fix 2: Unbound properties
text = text.replace("""@property (strong) NSTableView *roomTableView;
@property (strong) NSButton *privateCheckbox;
@property (strong) NSButton *okButton;""", "")

with open("main.mm", "w") as f:
    f.write(text)
