import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Replace the scrollbar calculation logic
old_calc_pattern = r'val exactIndex = firstItem\.index \+ \(-firstItem\.offset\.toFloat\(\) / firstItem\.size\.coerceAtLeast\(1\)\).*?var scrollFraction = \(exactIndex / maxIndex\)\.coerceIn\(0f, 1f\)'

new_calc = '''val scrolledDistance = (firstItem.index * cachedAverageItemSize) + (-firstItem.offset.toFloat())
            val maxScrollDistance = (estimatedTotalSize - viewportSize.toFloat()).coerceAtLeast(1f)
            var scrollFraction = (scrolledDistance / maxScrollDistance).coerceIn(0f, 1f)'''

content = re.sub(old_calc_pattern, new_calc, content, flags=re.DOTALL)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated scrollbar logic to use constant speed calculation")
