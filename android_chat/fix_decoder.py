import os

filepath = r'app/src/main/java/com/example/chat/media/VideoEngine.kt'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

old_code = '''    fun decodeH264(data: ByteArray) {
        Log.d("VideoEngine", "decodeH264 called with data size: ")
        val decoder = decoder ?: return
        try {
            val inputBufferId = decoder.dequeueInputBuffer(10000)
            if (inputBufferId >= 0) {
                val inputBuffer = decoder.getInputBuffer(inputBufferId)
                inputBuffer?.clear()
                inputBuffer?.put(data)
                decoder.queueInputBuffer(inputBufferId, 0, data.size, System.nanoTime() / 1000, 0)
            }'''

new_code = '''    fun decodeH264(data: ByteArray) {
        val decoder = decoder ?: return
        try {
            var naluData = data
            if (data.size > 4 && data[0] != 0.toByte() && data[0] != 1.toByte()) {
                // Could be length prefixed or missing start code. Check if it's AVCC.
                // We'll just enforce Annex-B start codes.
                if (!(data[0] == 0.toByte() && data[1] == 0.toByte() && (data[2] == 1.toByte() || (data[2] == 0.toByte() && data[3] == 1.toByte())))) {
                    // Try to assume it's length prefixed, replace first 4 bytes with 00 00 00 01
                    // Actually, let's just prepend 00 00 00 01 in case it's raw NAL unit
                    val startCode = byteArrayOf(0, 0, 0, 1)
                    val combined = ByteArray(data.size + 4)
                    System.arraycopy(startCode, 0, combined, 0, 4)
                    System.arraycopy(data, 0, combined, 4, data.size)
                    naluData = combined
                }
            }

            val inputBufferId = decoder.dequeueInputBuffer(10000)
            if (inputBufferId >= 0) {
                val inputBuffer = decoder.getInputBuffer(inputBufferId)
                inputBuffer?.clear()
                inputBuffer?.put(naluData)
                decoder.queueInputBuffer(inputBufferId, 0, naluData.size, System.nanoTime() / 1000, 0)
            }'''

if old_code in content:
    content = content.replace(old_code, new_code)
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Replaced decodeH264 logic successfully.")
else:
    print("Could not find old_code in VideoEngine.kt")
