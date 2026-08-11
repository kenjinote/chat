import json
log_file = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'
with open(log_file, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
            if data.get('step_index', 0) in [3785, 3786, 3787, 3788]:
                print(f"Step {data.get('step_index')}: {data.get('type')}")
                if data.get('type') == 'PLANNER_RESPONSE':
                    print(data.get('tool_calls'))
        except Exception as e:
            pass
