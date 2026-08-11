import json
log_file = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'
with open(log_file, 'r', encoding='utf-8') as f:
    for line in f:
        if '../recovered.kt' in line:
            data = json.loads(line)
            print(f"Step {data.get('step_index')}: {data.get('type')}")
