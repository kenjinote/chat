import json
import re

transcript_path = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'

target_call_id = None

with open(transcript_path, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
            
        if data.get('type') == 'PLANNER_RESPONSE' and data.get('step_index') == 4228:
            for tool in data.get('tool_calls', []):
                if tool['name'] == 'view_file':
                    target_call_id = tool.get('id')
                    print('Found call id:', target_call_id)
        
        if data.get('source') == 'SYSTEM' and data.get('type') == 'TOOL_RESPONSE' and target_call_id:
            # We don't have tool call ID in the response directly, it usually comes right after
            content = data.get('content', '')
            if '1:' in content and 'package com.example.chat' in content:
                print('Found response, length:', len(content))
                # Save it
                lines = content.split('\n')
                code_lines = []
                for l in lines:
                    match = re.match(r'^\s*\d+:\s(.*)', l)
                    if match:
                        code_lines.append(match.group(1))
                    elif l.startswith('  '): # Sometimes view_file wraps? No, it prefixes line numbers
                        pass
                
                with open('MainActivity_step4228.kt', 'w', encoding='utf-8') as out:
                    out.write('\n'.join(code_lines))
                print('Wrote to MainActivity_step4228.kt, line count:', len(code_lines))
                break
