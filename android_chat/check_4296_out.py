import json
log_file = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'
with open(log_file, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
            if data.get('type') == 'TOOL_RESPONSE' and data.get('tool_name') == 'run_command':
                if 'Applied ' in data.get('content', ''):
                    print(data.get('content'))
        except Exception as e:
            pass
