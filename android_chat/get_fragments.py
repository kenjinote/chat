import json
import re

transcript_path = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'

target_steps = [4229, 4232, 4259, 4262]
fragments = []

with open(transcript_path, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
        step = data.get('step_index', 0)
        
        if data.get('type') == 'VIEW_FILE' and step in target_steps:
            content = data.get('content', '')
            lines = content.split('\n')
            code_lines = []
            for l in lines:
                match = re.match(r'^\s*\d+:\s?(.*)', l)
                if match:
                    code_lines.append(match.group(1))
            fragments.append('\n'.join(code_lines))

with open('fragments.txt', 'w', encoding='utf-8') as f:
    f.write('\n\n--- FRAGMENT SEPARATOR ---\n\n'.join(fragments))
