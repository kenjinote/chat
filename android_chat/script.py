import json

log_file = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'
with open(log_file, 'r', encoding='utf-8') as f:
    lines = f.readlines()

for line in lines:
    try:
        data = json.loads(line)
        if data.get('type') == 'PLANNER_RESPONSE':
            for tc in data.get('tool_calls', []):
                name = tc.get('name')
                args = tc.get('args', {})
                if name == 'run_command':
                    cmd = args.get('CommandLine', '')
                    if 'git commit' in cmd or 'git add' in cmd:
                        print("GIT CMD:", cmd)
    except:
        pass
