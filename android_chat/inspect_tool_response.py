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
            
        if found_226 and data.get('source') == 'SYSTEM' and data.get('type') == 'TOOL_RESPONSE':
            print("Found TOOL_RESPONSE right after 226!")
            print("Has step_index:", data.get('step_index'))
            print("Content length:", len(data.get('content', '')))
            break
