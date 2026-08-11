import os

filepath = r'app/src/main/java/com/example/chat/media/VideoEngine.kt'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

old_code = '''    fun decodeH264(data: ByteArray) {
        val decoder = decoder ?: return
        try {
            var naluData = data'''

new_code = '''    fun decodeH264(data: ByteArray) {
        android.util.Log.d("VideoEngine", "decodeH264 called with size: " + data.size)
        if (data.size > 4) {
            android.util.Log.d("VideoEngine", "H264 prefix: " + data[0] + " " + data[1] + " " + data[2] + " " + data[3])
        }
        val decoder = decoder ?: return
        try {
            var naluData = data'''

if old_code in content:
    content = content.replace(old_code, new_code)
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Added logging to decodeH264.")
else:
    print("Could not find decodeH264 old_code.")
