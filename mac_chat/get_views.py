import json
with open("/Users/kenji/.gemini/antigravity/brain/da6765fd-c50d-4238-bc14-73c42302e28a/.system_generated/logs/transcript_full.jsonl", "r") as f:
    for line in f:
        try:
            data = json.loads(line)
            content = data.get("content", "")
            if "File Path: `file:///Users/kenji/Desktop/mac_chat/main.mm`" in content:
                for ln in content.split("\n"):
                    if "Showing lines" in ln:
                        print(ln)
        except:
            pass
