import json
import re

transcript_path = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'

with open(transcript_path, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
        step = data.get('step_index', 0)
        
        if data.get('type') == 'PLANNER_RESPONSE' and 380 < step < 470:
            for tool in data.get('tool_calls', []):
                args = tool.get('args', {})
                if isinstance(args, str):
                    try:
                        args = json.loads(args)
                    except:
                        continue
                if tool['name'] in ['replace_file_content', 'multi_replace_file_content']:
                    if tool['name'] == 'replace_file_content':
                        if 'nclr' in args.get('ReplacementContent', ''):
                            print(f"nclr replaced at step {step} (replace)")
                    else:
                        for chunk in args.get('ReplacementChunks', []):
                            if 'nclr' in chunk.get('ReplacementContent', ''):
                                print(f"nclr replaced at step {step} (multi)")
