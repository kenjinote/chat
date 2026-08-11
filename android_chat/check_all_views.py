import json
log_file = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'
with open(log_file, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
            if data.get('type') == 'PLANNER_RESPONSE':
                for tc in data.get('tool_calls', []):
                    if tc.get('name') == 'view_file':
                        args = tc.get('args', {})
                        if isinstance(args, str):
                            try:
                                args = json.loads(args)
                            except:
                                continue
                        if 'MainActivity.kt' in args.get('AbsolutePath', ''):
                            print(f"Step {data.get('step_index')}: {args.get('StartLine', 'None')} to {args.get('EndLine', 'None')}")
        except Exception as e:
            pass
