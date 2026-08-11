import json

transcript_path = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'

with open(transcript_path, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
        step = data.get('step_index', 0)
        
        if data.get('type') == 'PLANNER_RESPONSE' and 227 <= step < 470:
            for tool in data.get('tool_calls', []):
                if tool['name'] == 'invoke_subagent':
                    print(f"SUBAGENT at step {step}!")
