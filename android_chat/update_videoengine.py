import os

filepath = r'app/src/main/java/com/example/chat/media/VideoEngine.kt'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

old_code = '''    private var decoder: MediaCodec? = null
    private var configData: ByteArray? = null'''

new_code = '''    private var decoder: MediaCodec? = null
    private var configData: ByteArray? = null
    private var activeSurface: Surface? = null'''

if old_code in content:
    content = content.replace(old_code, new_code)
else:
    print("Could not find variables.")

old_code2 = '''    fun initDecoder(surface: Surface, width: Int = 640, height: Int = 480) {
        try {'''

new_code2 = '''    fun initDecoder(surface: Surface, width: Int = 640, height: Int = 480) {
        activeSurface = surface
        try {'''

if old_code2 in content:
    content = content.replace(old_code2, new_code2)

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

content += legacy_code

with open(filepath, 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated VideoEngine.kt")
