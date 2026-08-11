import re

path = 'app/src/main/java/com/example/chat/network/NetworkClient.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Replace sendTcpMessage body
pattern_send = r'fun sendTcpMessage\(message: ChatMessage\) \{\s*if \(tcpSocket == null \|\| tcpSocket\?\.isConnected == false\) return\s*try \{\s*val jsonStr = Json\.encodeToString\(message\)\s*tcpWriter\?\.println\(jsonStr\)\s*\} catch \(e: Exception\) \{\s*e\.printStackTrace\(\)\s*\}\s*\}'
replacement_send = '''fun sendTcpMessage(message: ChatMessage) {
        if (tcpSocket == null || tcpSocket?.isConnected == false) return
        try {
            val jsonStr = Json.encodeToString(message)
            val sjisBytes = jsonStr.toByteArray(java.nio.charset.Charset.forName("Shift_JIS"))
            tcpSocket!!.outputStream.write(sjisBytes)
            tcpSocket!!.outputStream.write('\\n'.code)
            tcpSocket!!.outputStream.flush()
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }'''

content = re.sub(pattern_send, replacement_send, content)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated sendTcpMessage")
