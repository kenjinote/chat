import json

transcript_path = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'

with open(transcript_path, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
        step = data.get('step_index', 0)
        
        if data.get('type') == 'PLANNER_RESPONSE' and 4200 <= step <= 4446:
            for tool in data.get('tool_calls', []):
                args = tool.get('args', {})
                if isinstance(args, str):
                    try:
                        args = json.loads(args)
                    except:
                        continue
                if tool['name'] in ['replace_file_content', 'multi_replace_file_content']:
                    print(f"Step {step}: {tool['name']}")
                    if tool['name'] == 'replace_file_content':
                        print("TARGET:")
                        print(args.get('TargetContent', ''))
                        print("REPLACEMENT:")
                        print(args.get('ReplacementContent', ''))
                    else:
                        for chunk in args.get('ReplacementChunks', []):
                            print("TARGET:")
                            print(chunk.get('TargetContent', ''))
                            print("REPLACEMENT:")
                            print(chunk.get('ReplacementContent', ''))
                    print("-" * 40)
