import os

filepath = r'app/src/main/java/com/example/chat/media/VideoEngine.kt'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

legacy_code = '''
    fun decodeLegacyVideo(data: ByteArray) {
        val surface = activeSurface ?: return
        try {
            val bitmap = android.graphics.BitmapFactory.decodeByteArray(data, 0, data.size)
            if (bitmap != null) {
                val canvas = surface.lockCanvas(null)
                if (canvas != null) {
                    val rect = android.graphics.Rect(0, 0, canvas.width, canvas.height)
                    canvas.drawBitmap(bitmap, null, rect, null)
                    surface.unlockCanvasAndPost(canvas)
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
'''

if legacy_code in content:
    content = content.replace(legacy_code, "")
    # insert before the last }
    last_brace_index = content.rfind('}')
    content = content[:last_brace_index] + legacy_code + content[last_brace_index:]
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Moved decodeLegacyVideo inside the class.")
else:
    print("Could not find legacy_code in VideoEngine.kt")
