import re

path = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Add win95Scrollbar modifier at the end of the file
scrollbar_code = '''
fun androidx.compose.ui.Modifier.win95Scrollbar(
    state: androidx.compose.foundation.lazy.LazyListState,
    width: androidx.compose.ui.unit.Dp = 16.dp
): androidx.compose.ui.Modifier = androidx.compose.ui.draw.drawWithContent {
    drawContent()
    val totalItems = state.layoutInfo.totalItemsCount
    val visibleItemsInfo = state.layoutInfo.visibleItemsInfo
    if (totalItems > 0 && visibleItemsInfo.isNotEmpty()) {
        val visibleItems = visibleItemsInfo.size
        val proportion = (visibleItems.toFloat() / totalItems.toFloat()).coerceIn(0f, 1f)
        if (proportion < 1f) {
            val scrollbarHeight = size.height
            val thumbHeight = (scrollbarHeight * proportion).coerceAtLeast(width.toPx())
            
            val firstItem = visibleItemsInfo.first()
            val itemSize = firstItem.size.coerceAtLeast(1)
            val offset = firstItem.offset
            val smoothIndex = firstItem.index - (offset.toFloat() / itemSize)
            
            val viewportSize = state.layoutInfo.viewportEndOffset - state.layoutInfo.viewportStartOffset
            val maxSmoothIndex = (totalItems - (viewportSize.toFloat() / itemSize)).coerceAtLeast(0f)
            
            val scrollFraction = if (maxSmoothIndex > 0) (smoothIndex / maxSmoothIndex).coerceIn(0f, 1f) else 0f
            val maxThumbOffset = scrollbarHeight - thumbHeight
            val thumbOffsetY = maxThumbOffset * scrollFraction
            
            val trackX = size.width - width.toPx()
            
            androidx.compose.ui.graphics.drawscope.drawIntoCanvas { canvas ->
                val paint = androidx.compose.ui.graphics.Paint().apply { color = androidx.compose.ui.graphics.Color(0xFFDFDFDF) }
                canvas.drawRect(androidx.compose.ui.geometry.Rect(trackX, 0f, size.width, scrollbarHeight), paint)
            }
            
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
'''
if "win95Scrollbar" not in content:
    content += "\n" + scrollbar_code

# 2. Update Chat Messages LazyColumn
chat_old = '''                // Left: Chat Messages
                Win95Panel(modifier = Modifier.weight(7f).fillMaxHeight(), backgroundColor = Color.White) {
                    LazyColumn(modifier = Modifier.fillMaxSize().background(Color.White).padding(0.5.dp)) {
                        items(messages) { msg ->'''

chat_new = '''                // Left: Chat Messages
                Win95Panel(modifier = Modifier.weight(7f).fillMaxHeight(), backgroundColor = Color.White) {
                    val chatListState = androidx.compose.foundation.lazy.rememberLazyListState()
                    val messagesSize = messages.size
                    val isAtBottom by remember {
                        derivedStateOf {
                            val lastVisibleItem = chatListState.layoutInfo.visibleItemsInfo.lastOrNull()
                            lastVisibleItem == null || lastVisibleItem.index >= chatListState.layoutInfo.totalItemsCount - 2
                        }
                    }
                    LaunchedEffect(messagesSize) {
                        if (messagesSize > 0 && isAtBottom) {
                            chatListState.animateScrollToItem(messagesSize - 1)
                        }
                    }
                    LazyColumn(state = chatListState, modifier = Modifier.fillMaxSize().background(Color.White).padding(0.5.dp).win95Scrollbar(chatListState)) {
                        items(messages) { msg ->'''

if "chatListState =" not in content:
    content = content.replace(chat_old, chat_new)

# 3. Update User List LazyColumn
user_old = '''                    Win95Panel(modifier = Modifier.weight(1f).fillMaxHeight(), backgroundColor = Color.White) {
                        LazyColumn(modifier = Modifier.fillMaxSize().background(Color.White).padding(0.5.dp)) {
                            items(users) { user ->'''

user_new = '''                    Win95Panel(modifier = Modifier.weight(1f).fillMaxHeight(), backgroundColor = Color.White) {
                        val userListState = androidx.compose.foundation.lazy.rememberLazyListState()
                        LazyColumn(state = userListState, modifier = Modifier.fillMaxSize().background(Color.White).padding(0.5.dp).win95Scrollbar(userListState)) {
                            items(users) { user ->'''

if "userListState =" not in content:
    content = content.replace(user_old, user_new)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated MainActivity.kt successfully!")
