import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Replace the scrollbar definition
old_code_pattern = r'fun androidx\.compose\.ui\.Modifier\.win95Scrollbar\(.*?\n\}\n'

new_code = '''fun androidx.compose.ui.Modifier.win95Scrollbar(
    state: androidx.compose.foundation.lazy.LazyListState,
    width: androidx.compose.ui.unit.Dp = 16.dp
): androidx.compose.ui.Modifier = this.drawWithContent {
    drawContent()
    val totalItems = state.layoutInfo.totalItemsCount
    val visibleItemsInfo = state.layoutInfo.visibleItemsInfo
    if (totalItems > 0 && visibleItemsInfo.isNotEmpty()) {
        val viewportSize = state.layoutInfo.viewportEndOffset - state.layoutInfo.viewportStartOffset
        val estimatedVisibleItems = (viewportSize.toFloat() / 60f).coerceAtLeast(1f)
        
        // Thumb size remains constant regardless of scroll position
        val proportion = (estimatedVisibleItems / totalItems.toFloat()).coerceIn(0.05f, 1f)
        
        val scrollbarHeight = size.height
        val thumbHeight = (scrollbarHeight * proportion).coerceAtLeast(width.toPx())
        
        val firstItem = visibleItemsInfo.first()
        val exactIndex = firstItem.index + (-firstItem.offset.toFloat() / firstItem.size.coerceAtLeast(1))
        
        val maxIndex = (totalItems - estimatedVisibleItems).coerceAtLeast(1f)
        var scrollFraction = (exactIndex / maxIndex).coerceIn(0f, 1f)
        
        val lastItem = visibleItemsInfo.last()
        if (lastItem.index == totalItems - 1 && lastItem.offset + lastItem.size <= viewportSize) {
            scrollFraction = 1f
        }
        
        val maxThumbOffset = scrollbarHeight - thumbHeight
        val thumbOffsetY = maxThumbOffset * scrollFraction
        
        val trackX = size.width - width.toPx()
        
        drawRect(
            color = androidx.compose.ui.graphics.Color(0xFFDFDFDF),
            topLeft = androidx.compose.ui.geometry.Offset(trackX, 0f),
            size = androidx.compose.ui.geometry.Size(width.toPx(), scrollbarHeight)
        )
        
        val thumbX = trackX
        val thumbY = thumbOffsetY
        val thumbW = width.toPx()
        val thumbH = thumbHeight
        
        drawRect(
            color = androidx.compose.ui.graphics.Color(0xFFC0C0C0),
            topLeft = androidx.compose.ui.geometry.Offset(thumbX, thumbY),
            size = androidx.compose.ui.geometry.Size(thumbW, thumbH)
        )
        
        val bw = 2.dp.toPx()
        drawRect(androidx.compose.ui.graphics.Color.White, topLeft = androidx.compose.ui.geometry.Offset(thumbX, thumbY), size = androidx.compose.ui.geometry.Size(thumbW, bw))
        drawRect(androidx.compose.ui.graphics.Color.White, topLeft = androidx.compose.ui.geometry.Offset(thumbX, thumbY), size = androidx.compose.ui.geometry.Size(bw, thumbH))
        drawRect(androidx.compose.ui.graphics.Color.Black, topLeft = androidx.compose.ui.geometry.Offset(thumbX, thumbY + thumbH - bw), size = androidx.compose.ui.geometry.Size(thumbW, bw))
        drawRect(androidx.compose.ui.graphics.Color.Black, topLeft = androidx.compose.ui.geometry.Offset(thumbX + thumbW - bw, thumbY), size = androidx.compose.ui.geometry.Size(bw, thumbH))
    }
}
'''

content = re.sub(old_code_pattern, new_code, content, flags=re.DOTALL)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated scrollbar size calculation")
