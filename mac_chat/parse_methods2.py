with open("deleted_lines.m", "r") as f:
    lines = f.readlines()

for i, line in enumerate(lines):
    if "applicationDidFinishLaunching" in line or "layoutControls" in line or "onSendClicked" in line or "onFormatChanged" in line or "onVoiceClicked" in line:
        print(f"--- Found at {i} ---")
        for j in range(max(0, i-5), min(len(lines), i+20)):
            print(lines[j], end="")
