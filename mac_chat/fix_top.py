import re

with open("main.mm", "r") as f:
    text = f.read()

text = text.replace("""        for (int j=0; j<2; j++) {
            if (ABS(y_cands[j]) < snapEdge && ABS(y_cands[j]) < ABS(minDy)) {
                minDy = y_cands[j];
            }
}
@end""", """        for (int j=0; j<2; j++) {
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
@end""")

text = text.replace("""        }
    }
    
    if (cornerSnapped) {
        snappedFrame.origin.x -= minDx;
        snappedFrame.origin.y -= minDy;
    }
    
    [super setFrame:snappedFrame display:flag];
    self.isProgrammaticSet = NO;
}
""", "")

with open("main.mm", "w") as f:
    f.write(text)
