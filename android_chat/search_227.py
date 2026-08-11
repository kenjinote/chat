import json
import re

transcript_path = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'

with open(transcript_path, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
                    
        if data.get('step_index') == 227:
            content = data.get('content', '')
            if 'java.util.concurrent.Executors' in content:
                print("Found in step 227!")
            else:
                print("NOT FOUND in step 227!")
            break
