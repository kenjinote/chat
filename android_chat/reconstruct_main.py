import json
import re

transcript_path = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'

current_content = ""
# 1. get the full view_file at step 227
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
                    match = re.match(r'^\s*\d+:\s?(.*)', l)
                    if match:
                        code_lines.append(match.group(1))
                current_content = '\n'.join(code_lines).replace('\r', '')
                break

print('Initial content length:', len(current_content))

# 2. apply replacements
failed_steps = []
with open(transcript_path, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
        step = data.get('step_index', 0)
        if data.get('type') == 'PLANNER_RESPONSE' and 227 <= step <= 3389:
            for tool in data.get('tool_calls', []):
                args = tool.get('args', {})
                if isinstance(args, str):
                    try:
                        args = json.loads(args)
                    except:
                        continue
                if tool['name'] == 'replace_file_content':
                    if 'MainActivity.kt' in args.get('TargetFile', ''):
                        target = args.get('TargetContent', '').replace('\r', '')
                        replacement = args.get('ReplacementContent', '').replace('\r', '')
                        if target in current_content:
                            current_content = current_content.replace(target, replacement, 1)
                        else:
                            failed_steps.append(f'Step {step} (replace)')
                elif tool['name'] == 'multi_replace_file_content':
                    if 'MainActivity.kt' in args.get('TargetFile', ''):
                        chunks = args.get('ReplacementChunks', [])
                        for chunk in chunks:
                            target = chunk.get('TargetContent', '').replace('\r', '')
                            replacement = chunk.get('ReplacementContent', '').replace('\r', '')
                            if target in current_content:
                                current_content = current_content.replace(target, replacement, 1)
                            else:
                                failed_steps.append(f'Step {step} (multi)')

with open('MainActivity_recovered_clean.kt', 'w', encoding='utf-8') as f:
    f.write(current_content)
print('Recovered file length:', len(current_content))
print('Failed steps:', len(failed_steps))
if len(failed_steps) > 0:
    print('First few failed:', failed_steps[:10])
