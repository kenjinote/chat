import json
import re

transcript_path = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'

with open(transcript_path, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
            
        if data.get('type') == 'PLANNER_RESPONSE':
            for tool in data.get('tool_calls', []):
                if tool['name'] == 'view_file':
                    args = tool.get('args', {})
                    if isinstance(args, str):
                        try:
                            args = json.loads(args)
                        except:
                            continue
                    if 'MainActivity.kt' in args.get('AbsolutePath', ''):
                        print('Found view_file call for MainActivity.kt at step', data.get('step_index'))
