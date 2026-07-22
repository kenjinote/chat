import re

with open("main.mm", "r") as f:
    content = f.read()

# 1. Update initial title
old_initial_title = """    self.voiceButton.title = [NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_TALK", nil)];"""
new_initial_title = """    self.voiceButton.title = NSLocalizedString(@"IDS_TALK", nil);"""
content = content.replace(old_initial_title, new_initial_title)

# 2. Update onVoiceClicked
old_on_voice = """- (void)onVoiceClicked:(id)sender {
    if ([self.voiceButton.title isEqualToString:[NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_TALK", nil)]]) {
        self.voiceButton.title = [NSString stringWithFormat:@"%@: ON", NSLocalizedString(@"IDS_TALK", nil)];
        if (audioEngine) audioEngine->StartMicrophone();
    } else {
        self.voiceButton.title = [NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_TALK", nil)];
        if (audioEngine) audioEngine->StopMicrophone();
        self.micVolumeIndicator.doubleValue = 0;
    }
}"""
new_on_voice = """- (void)onVoiceClicked:(id)sender {
    if (self.voiceButton.state == NSControlStateValueOn) {
        if (audioEngine) audioEngine->StartMicrophone();
    } else {
        if (audioEngine) audioEngine->StopMicrophone();
        self.micVolumeIndicator.doubleValue = 0;
    }
}"""
content = content.replace(old_on_voice, new_on_voice)

# 3. Adjust spacing in drawRect for mic icon
old_draw_mic = """    if (self.hasMicIcon) {
        CGFloat cx = bounds.size.width / 2.0 - 18;
        CGFloat cy = bounds.size.height / 2.0;
        if (pressed) { cx += 1; cy += 1; }
        
        [[NSColor blackColor] setStroke];
        [[NSColor colorWithCalibratedRed:0.2 green:0.2 blue:0.2 alpha:1.0] setFill];
        
        // mic head (capsule)
        NSRect capRect = NSMakeRect(cx - 3, cy - 5, 6, 10);
        NSBezierPath *cap = [NSBezierPath bezierPathWithRoundedRect:capRect xRadius:2 yRadius:2];
        [cap fill];
        
        // U shape
        NSBezierPath *u = [NSBezierPath bezierPath];
        [u moveToPoint:NSMakePoint(cx - 5, cy)];
        [u lineToPoint:NSMakePoint(cx - 5, cy + 5)];
        [u lineToPoint:NSMakePoint(cx + 5, cy + 5)];
        [u lineToPoint:NSMakePoint(cx + 5, cy)];
        [u setLineWidth:1.5];
        [u stroke];
        
        // stem and base
        NSBezierPath *stem = [NSBezierPath bezierPath];
        [stem moveToPoint:NSMakePoint(cx, cy + 5)];
        [stem lineToPoint:NSMakePoint(cx, cy + 9)];
        [stem moveToPoint:NSMakePoint(cx - 4, cy + 9)];
        [stem lineToPoint:NSMakePoint(cx + 4, cy + 9)];
        [stem setLineWidth:1.5];
        [stem stroke];
        
        titleRect.origin.x += 8;
    }"""
    
new_draw_mic = """    if (self.hasMicIcon) {
        // Adjust for shorter title "話す" to prevent overlapping
        CGFloat cx = bounds.size.width / 2.0 - 20;
        CGFloat cy = bounds.size.height / 2.0;
        if (pressed) { cx += 1; cy += 1; }
        
        [[NSColor blackColor] setStroke];
        [[NSColor colorWithCalibratedRed:0.2 green:0.2 blue:0.2 alpha:1.0] setFill];
        
        // mic head (capsule)
        NSRect capRect = NSMakeRect(cx - 3, cy - 5, 6, 10);
        NSBezierPath *cap = [NSBezierPath bezierPathWithRoundedRect:capRect xRadius:2 yRadius:2];
        [cap fill];
        
        // U shape
        NSBezierPath *u = [NSBezierPath bezierPath];
        [u moveToPoint:NSMakePoint(cx - 5, cy)];
        [u lineToPoint:NSMakePoint(cx - 5, cy + 5)];
        [u lineToPoint:NSMakePoint(cx + 5, cy + 5)];
        [u lineToPoint:NSMakePoint(cx + 5, cy)];
        [u setLineWidth:1.5];
        [u stroke];
        
        // stem and base
        NSBezierPath *stem = [NSBezierPath bezierPath];
        [stem moveToPoint:NSMakePoint(cx, cy + 5)];
        [stem lineToPoint:NSMakePoint(cx, cy + 9)];
        [stem moveToPoint:NSMakePoint(cx - 4, cy + 9)];
        [stem lineToPoint:NSMakePoint(cx + 4, cy + 9)];
        [stem setLineWidth:1.5];
        [stem stroke];
        
        titleRect.origin.x += 12;
    }"""
content = content.replace(old_draw_mic, new_draw_mic)

with open("main.mm", "w") as f:
    f.write(content)
