import json
log_file = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'
with open(log_file, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
            if data.get('step_index') == 2089:
                for tc in data.get('tool_calls', []):
                    args = tc.get('args', {})
                    if isinstance(args, str):
                        try:
                            args = json.loads(args)
                        except:
                            continue
                    print(args.get('CommandLine', ''))
        except Exception as e:
            pass
