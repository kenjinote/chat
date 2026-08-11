import re

path = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

helper = '''
    private fun fixString(s: String?): String? {
        if (s == null) return null
        return try {
            val latin1Bytes = s.toByteArray(java.nio.charset.Charset.forName("ISO-8859-1"))
            String(latin1Bytes, java.nio.charset.Charset.forName("Shift_JIS"))
        } catch (e: Exception) {
            s
        }
    }
'''

# Find the class opening
class_pattern = r'(class ChatViewModel[^{]*\{)'
content = re.sub(class_pattern, r'\1' + helper, content)

# Now fix the message strings in handleIncomingMessage
# It's better to just fix them before the switch
switch_pattern = r'(private fun handleIncomingMessage\(msg: ChatMessage\): Boolean \{\s*)(when \(msg\.type\) \{)'

fix_logic = '''
        var message = msg
        message = message.copy(
            roomName = fixString(message.roomName),
            userName = fixString(message.userName),
            msg = fixString(message.msg),
            content = fixString(message.content),
            users = message.users?.map { it.copy(userName = fixString(it.userName) ?: it.userName) }
        )
        val msg_orig = msg
        val msg = message
'''

content = re.sub(switch_pattern, r'\1' + fix_logic + r'\n        \2', content)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated ChatViewModel.kt")
