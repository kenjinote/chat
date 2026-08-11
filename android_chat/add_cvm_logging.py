import os

filepath = r'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

old_code = '''                    } else if ((header.payloadType == MediaHeader.VIDEO || header.payloadType == MediaHeader.VIDEO_H264 || header.payloadType == MediaHeader.STREAM_STATUS) && header.userId != myUserId) {
                        _videoStreamers.value = _videoStreamers.value + (header.userId to System.currentTimeMillis())
                        if (_watchingUserId.value == header.userId || _watchingUserId.value == myUserId && header.userId == myUserId) {'''

new_code = '''                    } else if ((header.payloadType == MediaHeader.VIDEO || header.payloadType == MediaHeader.VIDEO_H264 || header.payloadType == MediaHeader.STREAM_STATUS) && header.userId != myUserId) {
                        if (header.payloadType != MediaHeader.STREAM_STATUS) {
                            android.util.Log.d("ChatViewModel", "Received VIDEO packet from " + header.userId + ", watching: " + _watchingUserId.value + ", type: " + header.payloadType)
                        }
                        _videoStreamers.value = _videoStreamers.value + (header.userId to System.currentTimeMillis())
                        if (_watchingUserId.value == header.userId || _watchingUserId.value == myUserId && header.userId == myUserId) {'''

if old_code in content:
    content = content.replace(old_code, new_code)
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Added logging to ChatViewModel.")
else:
    print("Could not find ChatViewModel old_code.")
