import json
import re

transcript_path = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'

user_msg = ""
with open(transcript_path, 'r', encoding='utf-8') as f:
    lines = f.readlines()
    for line in reversed(lines):
        data = json.loads(line)
        if data.get('type') == 'USER_INPUT':
            user_msg = data.get('content', '')
            break

logs = []
for line in user_msg.split('\n'):
    if 'VideoEngine' in line or 'ChatViewModel' in line or 'Win95VideoWindow' in line:
        logs.append(line)

print("Found relevant logs:")
for log in logs:
    print(log.strip())
