import re

with open("diff_output.txt", "r") as f:
    text = f.read()

blocks = text.split("FOUND-------------------------")
last_block = blocks[-1]
diff_start = last_block.find("[diff_block_start]")
diff_end = last_block.find("[diff_block_end]")
if diff_end == -1: diff_end = len(last_block)
diff_text = last_block[diff_start + len("[diff_block_start]"):diff_end].strip()

# Clean up truncated line
diff_lines = diff_text.splitlines()
clean_lines = []
for line in diff_lines:
    if "<truncated" in line:
        break
    clean_lines.append(line)

# Remove the last hunk if it's incomplete
# A hunk is incomplete if it doesn't have the declared number of lines.
# But `patch` can ignore malformed hunks if we use `--force` or just let it fail on that hunk.
# Let's just remove the last hunk to be safe.
hunk_starts = [i for i, line in enumerate(clean_lines) if line.startswith("@@ ")]
if len(hunk_starts) > 1:
    clean_lines = clean_lines[:hunk_starts[-1]]

with open("main_fix.patch", "w") as f:
    f.write("--- main.mm\n")
    f.write("+++ main.mm\n")
    for line in clean_lines:
        f.write(line + "\n")
