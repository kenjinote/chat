import json
import re

transcript_path = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'

with open(transcript_path, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
                    
        if data.get('step_index') == 227:
            content = data.get('content', '')
            if '1:' in content and 'package com.example.chat' in content:
                lines = content.split('\n')
                code_lines = []
                for l in lines:
                    match = re.match(r'^\s*\d+:\s(.*)', l)
                    if match:
                        code_lines.append(match.group(1))
                
                print('\n'.join(code_lines[:20]))
                break
