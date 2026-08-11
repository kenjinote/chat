import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Update ScrollbarCache class
old_cache_class = '''class ScrollbarCache {
    val itemHeights = mutableMapOf<Int, Int>()
    var totalCachedHeight = 0
    var lastTotalItems = 0
    var wasAtBottom = false
    var lastAbsoluteY = 0f
}'''
new_cache_class = '''class ScrollbarCache {
    val itemHeights = mutableMapOf<Int, Int>()
    var totalCachedHeight = 0
    var lastTotalItems = 0
    var wasAtBottom = false
    var lastAbsoluteY = 0f
    var isAutoScrollLock = false
}'''
content = content.replace(old_cache_class, new_cache_class)

# 2. Update the logic in drawWithContent
# First, remove the old lastTotalItems update at the top
old_top = '''if (totalItems < cache.lastTotalItems) {
                cache.itemHeights.clear()
                cache.totalCachedHeight = 0
            }
            cache.lastTotalItems = totalItems'''
new_top = '''if (totalItems < cache.lastTotalItems) {
                cache.itemHeights.clear()
                cache.totalCachedHeight = 0
            }
            val newItemsAdded = totalItems > cache.lastTotalItems'''
content = content.replace(old_top, new_top)

# Next, replace the old scrollFraction logic
old_calc_pattern = r'val lastItem = visibleItemsInfo\.last\(\)\s*val isCurrentlyAtBottom = .*?cache\.lastAbsoluteY = absoluteY'
new_calc = '''val lastItem = visibleItemsInfo.last()
            val isCurrentlyAtBottom = (lastItem.index == totalItems - 1 && lastItem.offset + lastItem.size <= viewportSize + 1f)
            
            if (isCurrentlyAtBottom) {
                scrollFraction = 1f
                cache.wasAtBottom = true
                cache.isAutoScrollLock = false
            } else {
                if (cache.wasAtBottom && newItemsAdded) {
                    cache.isAutoScrollLock = true
                    scrollFraction = 1f
                } else if (cache.isAutoScrollLock && Math.abs(absoluteY - cache.lastAbsoluteY) < 1f) {
                    scrollFraction = 1f
                } else {
                    scrollFraction = (absoluteY / maxScrollDistance).coerceIn(0f, 1f)
                    cache.wasAtBottom = false
                    cache.isAutoScrollLock = false
                }
            }
            cache.lastAbsoluteY = absoluteY
            cache.lastTotalItems = totalItems'''

content = re.sub(old_calc_pattern, new_calc, content, flags=re.DOTALL)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated scroll logic with proper lock breaking")
