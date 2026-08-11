import json
import os
import subprocess

log_file = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'
target_file = 'app/src/main/java/com/example/chat/MainActivity.kt'

subprocess.run(['git', 'restore', target_file])

with open(target_file, 'r', encoding='utf-8') as f:
    file_content = f.read().replace('\r\n', '\n')

applied = 0
failed = 0

with open(log_file, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
            
        step = data.get('step_index', 0)
        
        if step > 3800:
            break
            
        if data.get('type') == 'PLANNER_RESPONSE':
            for tc in data.get('tool_calls', []):
                args = tc.get('args', {})
                if isinstance(args, str):
                    try:
                        args = json.loads(args)
                    except:
                        continue
                
                name = tc.get('name')
                if name == 'replace_file_content' and 'MainActivity.kt' in args.get('TargetFile', ''):
                    target = args.get('TargetContent', '').replace('\r\n', '\n')
                    replacement = args.get('ReplacementContent', '').replace('\r\n', '\n')
                    
                    if file_content.count(target) == 1:
                        file_content = file_content.replace(target, replacement)
                        applied += 1
                    else:
                        failed += 1
                        
                elif name == 'multi_replace_file_content' and 'MainActivity.kt' in args.get('TargetFile', ''):
                    chunks = args.get('ReplacementChunks', [])
                    for chunk in chunks:
                        target = chunk.get('TargetContent', '').replace('\r\n', '\n')
                        replacement = chunk.get('ReplacementContent', '').replace('\r\n', '\n')
                        if file_content.count(target) == 1:
                            file_content = file_content.replace(target, replacement)
                            applied += 1
                        else:
                            failed += 1

with open(target_file, 'w', encoding='utf-8') as f:
    f.write(file_content)

print(f"Applied {applied} replacements, failed {failed}.")
