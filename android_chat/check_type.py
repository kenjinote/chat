import json
log_file = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'
with open(log_file, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
            content = data.get('content', '')
            if isinstance(content, str) and '[diff_block_start]' in content:
                print("FOUND TYPE:", data.get('type'))
                print(content[:200])
                break
        except Exception as e:
            pass
