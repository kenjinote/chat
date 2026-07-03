with open('Source.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Truncate after line 1625
lines = lines[:1625]
content = "".join(lines)

# Now move audio_globals down.
start_marker = "// --- Audio Globals ---"
end_marker = "waveOutWrite(g_hWaveOut, &g_waveOutHdr[freeIdx], sizeof(WAVEHDR));\n}"

start_idx = content.find(start_marker)
end_idx = content.find(end_marker) + len(end_marker)

audio_code = content[start_idx:end_idx]
content = content[:start_idx] + content[end_idx:]

insert_marker = "IDWriteFactory* g_pDWriteFactory = nullptr;"
insert_idx = content.find(insert_marker) + len(insert_marker)

content = content[:insert_idx] + "\n\n" + audio_code + "\n" + content[insert_idx:]

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Source fixed")
