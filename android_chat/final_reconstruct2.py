import json
import re

transcript_path = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'

def fuzzy_replace(content, target, replacement):
    def tokenize(text):
        return [t for t in re.split(r'(\W+)', text) if t]
    
    c_tokens = tokenize(content)
    t_tokens = tokenize(target)
    
    t_nonws = [t for t in t_tokens if not re.match(r'^\s*$', t)]
    c_nonws = [t for t in c_tokens if not re.match(r'^\s*$', t)]
    
    if not t_nonws:
        return content, False
        
    t_len = len(t_nonws)
    c_len = len(c_nonws)
    
    match_start_idx = -1
    for i in range(c_len - t_len + 1):
        match = True
        for j in range(t_len):
            if c_nonws[i+j] != t_nonws[j]:
                match = False
                break
        if match:
            match_start_idx = i
            break
            
    if match_start_idx == -1:
        return content, False
        
    nonws_count = 0
    start_char_idx = -1
    for i, t in enumerate(c_tokens):
        if not re.match(r'^\s*$', t):
            if nonws_count == match_start_idx:
                start_char_idx = sum(len(x) for x in c_tokens[:i])
                break
            nonws_count += 1
            
    nonws_count = 0
    end_char_idx = -1
    for i, t in enumerate(c_tokens):
        if not re.match(r'^\s*$', t):
            if nonws_count == match_start_idx + t_len - 1:
                end_char_idx = sum(len(x) for x in c_tokens[:i+1])
                break
            nonws_count += 1

    if start_char_idx != -1 and end_char_idx != -1:
        new_content = content[:start_char_idx] + replacement + content[end_char_idx:]
        return new_content, True
    
    return content, False

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
                current_content = '\n'.join(code_lines)
                break

step_success = {}
last_step = -1
with open(transcript_path, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
        step = data.get('step_index', 0)
        
        if data.get('type') == 'PLANNER_RESPONSE':
            last_step = step
            
        if data.get('source') == 'MODEL' and data.get('type') not in ['PLANNER_RESPONSE', 'USER_INPUT'] and last_step != -1:
            content = data.get('content', '')
            if 'The following changes were made' in content:
                step_success[last_step] = True
            else:
                step_success[last_step] = False

failed_script_steps = []
with open(transcript_path, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
        step = data.get('step_index', 0)
        if data.get('type') == 'PLANNER_RESPONSE' and 227 <= step <= 3389:
            if not step_success.get(step, False):
                continue
                
            for tool in data.get('tool_calls', []):
                args = tool.get('args', {})
                if isinstance(args, str):
                    try:
                        args = json.loads(args)
                    except:
                        continue
                if tool['name'] == 'replace_file_content':
                    if 'MainActivity.kt' in args.get('TargetFile', ''):
                        target = args.get('TargetContent', '')
                        replacement = args.get('ReplacementContent', '')
                        current_content, success = fuzzy_replace(current_content, target, replacement)
                        if not success:
                            failed_script_steps.append(f'Step {step} (replace)')
                elif tool['name'] == 'multi_replace_file_content':
                    if 'MainActivity.kt' in args.get('TargetFile', ''):
                        chunks = args.get('ReplacementChunks', [])
                        for chunk in chunks:
                            target = chunk.get('TargetContent', '')
                            replacement = chunk.get('ReplacementContent', '')
                            current_content, success = fuzzy_replace(current_content, target, replacement)
                            if not success:
                                failed_script_steps.append(f'Step {step} (multi)')

with open('MainActivity_final2.kt', 'w', encoding='utf-8') as f:
    f.write(current_content)

print('Recovered file length:', len(current_content))
print('Script failures on successful original steps:', len(failed_script_steps))
if len(failed_script_steps) > 0:
    print('First few failed:', failed_script_steps[:10])
