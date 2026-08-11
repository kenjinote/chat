import json

transcript_path = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'
found = False
count = 0

with open(transcript_path, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
        step = data.get('step_index', 0)
        
        if data.get('type') == 'PLANNER_RESPONSE' and step == 470:
            found = True
            print("Found 470")
            continue
            
        if found:
            print(f"Step {step}: type={data.get('type')}, source={data.get('source')}")
            count += 1
            if count >= 5:
                break
