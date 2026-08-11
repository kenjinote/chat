import json

transcript_path = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'

with open(transcript_path, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
        step = data.get('step_index', 0)
        
        if 470 <= step <= 475:
            print(f"Step {step}: type={data.get('type')}, source={data.get('source')}")
            if data.get('source') == 'MODEL' and data.get('type') not in ['PLANNER_RESPONSE']:
                content = data.get('content', '')
                print(f"   Content starts with: {content[:100]}")
