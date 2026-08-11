import os

filepath = r'app/src/main/java/com/example/chat/media/VideoEngine.kt'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

old_code = '''    fun decodeLegacyVideo(data: ByteArray) {
        val surface = activeSurface ?: return
        try {
            val bitmap = android.graphics.BitmapFactory.decodeByteArray(data, 0, data.size)
            if (bitmap != null) {'''

new_code = '''    fun decodeLegacyVideo(data: ByteArray) {
        android.util.Log.d("VideoEngine", "decodeLegacyVideo called with size: " + data.size)
        if (data.size > 4) {
            android.util.Log.d("VideoEngine", "Legacy video prefix: " + data[0] + " " + data[1] + " " + data[2] + " " + data[3])
        }
        val surface = activeSurface ?: return
        try {
            val bitmap = android.graphics.BitmapFactory.decodeByteArray(data, 0, data.size)
            if (bitmap != null) {
                android.util.Log.d("VideoEngine", "Legacy video bitmap decoded successfully")'''

if old_code in content:
    content = content.replace(old_code, new_code)
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Added logging to decodeLegacyVideo.")
else:
    print("Could not find decodeLegacyVideo old_code.")
