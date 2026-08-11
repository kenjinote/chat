import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Update ScrollbarCache class
old_cache_class = '''class ScrollbarCache {
    val itemHeights = mutableMapOf<Int, Int>()
    var totalCachedHeight = 0
    var lastTotalItems = 0
}'''
new_cache_class = '''class ScrollbarCache {
    val itemHeights = mutableMapOf<Int, Int>()
    var totalCachedHeight = 0
    var lastTotalItems = 0
    var wasAtBottom = false
    var lastAbsoluteY = 0f
}'''
content = content.replace(old_cache_class, new_cache_class)

# 2. Update scrollFraction calculation logic
old_calc_pattern = r'val lastItem = visibleItemsInfo\.last\(\)\s*if \(lastItem\.index == totalItems - 1 && lastItem\.offset \+ lastItem\.size <= viewportSize\) \{\s*scrollFraction = 1f\s*\}'
new_calc = '''val lastItem = visibleItemsInfo.last()
            val isCurrentlyAtBottom = (lastItem.index == totalItems - 1 && lastItem.offset + lastItem.size <= viewportSize + 1f)
            
            if (isCurrentlyAtBottom) {
                scrollFraction = 1f
                cache.wasAtBottom = true
            } else {
                if (cache.wasAtBottom && absoluteY >= cache.lastAbsoluteY - 1f) {
                    scrollFraction = 1f
                } else {
                    scrollFraction = (absoluteY / maxScrollDistance).coerceIn(0f, 1f)
                    cache.wasAtBottom = false
                }
            }
            cache.lastAbsoluteY = absoluteY'''

content = re.sub(old_calc_pattern, new_calc, content, flags=re.DOTALL)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated scrollFraction logic with wasAtBottom lock")
