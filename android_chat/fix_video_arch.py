import re

path = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Remove processEncoderOutput from init {}
process_regex = re.compile(r'(\s*videoEngine\.processEncoderOutput \{.*?\n\s*\})', re.DOTALL)
content = process_regex.sub('', content)

# 2. But how do we link networkClient.sendUdpPacket to VideoEngine?
# We can expose a callback in VideoEngine.
