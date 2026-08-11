import json

transcript_path = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'

found_226 = False
with open(transcript_path, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
        
        if data.get('type') == 'PLANNER_RESPONSE' and data.get('step_index') == 226:
            found_226 = True
            print("Found PLANNER_RESPONSE at 226")
            continue
            
        if found_226:
            print(f"Next entry type: {data.get('type')}, source: {data.get('source')}, step_index: {data.get('step_index')}")
            content = data.get('content', '')
            print("Content start:", content[:100])
            break
