import json

transcript_path = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'

with open(transcript_path, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
        step = data.get('step_index', 0)
        
        if data.get('type') == 'PLANNER_RESPONSE' and step == 470:
            for tool in data.get('tool_calls', []):
                args = tool.get('args', {})
                if isinstance(args, str):
                    try:
                        args = json.loads(args)
                    except:
                        continue
                if tool['name'] == 'replace_file_content':
                    print("REPLACE:")
                    print(args.get('TargetContent', ''))
                elif tool['name'] == 'multi_replace_file_content':
                    for i, chunk in enumerate(args.get('ReplacementChunks', [])):
                        print(f"CHUNK {i} TARGET:")
                        print(chunk.get('TargetContent', ''))
