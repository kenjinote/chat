import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Replace the scrollbar calculation logic completely
old_code_pattern = r'fun androidx\.compose\.ui\.Modifier\.win95Scrollbar\(.*?\n\}\n'

new_code = '''class ScrollbarCache {
    val itemHeights = mutableMapOf<Int, Int>()
    var totalCachedHeight = 0
    var lastTotalItems = 0
}

fun androidx.compose.ui.Modifier.win95Scrollbar(
    state: androidx.compose.foundation.lazy.LazyListState,
    width: androidx.compose.ui.unit.Dp = 16.dp
): androidx.compose.ui.Modifier = composed {
    val cache = remember { ScrollbarCache() }

    this.drawWithContent {
        drawContent()
        val totalItems = state.layoutInfo.totalItemsCount
        val visibleItemsInfo = state.layoutInfo.visibleItemsInfo
        
        if (totalItems > 0 && visibleItemsInfo.isNotEmpty()) {
            val viewportSize = state.layoutInfo.viewportEndOffset - state.layoutInfo.viewportStartOffset.toFloat()
            
            if (totalItems < cache.lastTotalItems) {
                cache.itemHeights.clear()
                cache.totalCachedHeight = 0
            }
            cache.lastTotalItems = totalItems

            visibleItemsInfo.forEach { item ->
                val oldSize = cache.itemHeights[item.index]
                if (oldSize != item.size) {
                    cache.itemHeights[item.index] = item.size
                    cache.totalCachedHeight += item.size - (oldSize ?: 0)
                }
            }
            
            val cachedCount = cache.itemHeights.size
            val averageItemHeight = if (cachedCount > 0) {
                cache.totalCachedHeight.toFloat() / cachedCount
            } else {
                60f
            }
            
            val uncachedCount = (totalItems - cachedCount).coerceAtLeast(0)
            val totalEstimatedHeight = cache.totalCachedHeight.toFloat() + (uncachedCount * averageItemHeight)
            
            val proportion = (viewportSize / totalEstimatedHeight).coerceIn(0.05f, 1f)
            
            val scrollbarHeight = size.height
            val thumbHeight = (scrollbarHeight * proportion).coerceAtLeast(width.toPx())
            
            val firstItem = visibleItemsInfo.first()
            var absoluteY = 0f
            for (i in 0 until firstItem.index) {
                absoluteY += (cache.itemHeights[i] ?: averageItemHeight.toInt()).toFloat()
            }
            absoluteY += -firstItem.offset.toFloat()
            
            val maxScrollDistance = (totalEstimatedHeight - viewportSize).coerceAtLeast(1f)
            var scrollFraction = (absoluteY / maxScrollDistance).coerceIn(0f, 1f)
            
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
            
            if (proportion < 1f) {
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
    }
}
'''

content = re.sub(old_code_pattern, new_code, content, flags=re.DOTALL)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated scrollbar logic to use ScrollbarCache")
