import json

with open("/Users/kenji/.gemini/antigravity/brain/da6765fd-c50d-4238-bc14-73c42302e28a/.system_generated/logs/transcript_full.jsonl", "r") as f:
    for line in f:
        try:
            data = json.loads(line)
            content = data.get("content", "")
            if "The following changes were made by the multi_replace_file_content tool to: /Users/kenji/Desktop/mac_chat/main.mm" in content:
                print("FOUND-------------------------")
                print(content)
        except Exception as e:
            pass
