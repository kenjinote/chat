import os

filepath = r'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

old_code = '''                            if (header.payloadType == MediaHeader.VIDEO_H264) {
                                Log.d("ChatViewModel", "Received UDP video packet, size: ")
                                videoEngine.decodeH264(payload)
                            }'''

new_code = '''                            if (header.payloadType == MediaHeader.VIDEO_H264) {
                                Log.d("ChatViewModel", "Received UDP video packet, size: ")
                                videoEngine.decodeH264(payload)
                            } else if (header.payloadType == MediaHeader.VIDEO) {
                                videoEngine.decodeLegacyVideo(payload)
                            }'''

if old_code in content:
    content = content.replace(old_code, new_code)
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Added legacy video handler to ChatViewModel")
else:
    print("Could not find old_code in ChatViewModel")
