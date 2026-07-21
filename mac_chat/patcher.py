import re
import sys

def parse_unified_diff(diff_text):
    chunks = []
    current_chunk = None
    
    for line in diff_text.splitlines():
        if line.startswith('@@'):
            m = re.match(r'@@ -(\d+)(?:,\d+)? \+(\d+)(?:,\d+)? @@', line)
            if m:
                if current_chunk:
                    chunks.append(current_chunk)
                current_chunk = {'old_start': int(m.group(1)), 'new_start': int(m.group(2)), 'lines': []}
        elif current_chunk is not None:
            current_chunk['lines'].append(line)
            
    if current_chunk:
        chunks.append(current_chunk)
    return chunks

with open("diff_output.txt", "r") as f:
    text = f.read()

blocks = text.split("FOUND-------------------------")
last_block = blocks[-1]
diff_start = last_block.find("[diff_block_start]")
diff_end = last_block.find("[diff_block_end]")
if diff_end == -1: diff_end = len(last_block)

diff_text = last_block[diff_start + len("[diff_block_start]"):diff_end]
chunks = parse_unified_diff(diff_text)

# We want to REVERSE the diff.
# For each chunk, the "new" file (which is currently main.mm) has the lines corresponding to '+' and ' '
# We want to replace them with the lines corresponding to '-' and ' '.

with open("main.mm", "r") as f:
    original_lines = f.read().splitlines()

# We will apply chunks from bottom to top to not mess up line numbers.
chunks.sort(key=lambda x: x['new_start'], reverse=True)

for c in chunks:
    # Identify the block in the current file to replace.
    # It starts at new_start - 1
    # It consists of all '+' and ' ' lines in the chunk.
    # We will replace it with '-' and ' ' lines.
    
    # Wait, because of truncation, a chunk might be incomplete.
    # If a chunk is incomplete, we just take as much as we can.
    new_lines_in_chunk = []
    old_lines_in_chunk = []
    for line in c['lines']:
        if line.startswith('+'):
            new_lines_in_chunk.append(line[1:])
        elif line.startswith('-'):
            old_lines_in_chunk.append(line[1:])
        elif line.startswith(' '):
            new_lines_in_chunk.append(line[1:])
            old_lines_in_chunk.append(line[1:])
        else:
            # truncated or malformed, break
            break
            
    start_idx = c['new_start'] - 1
    # Replace from start_idx to start_idx + len(new_lines_in_chunk)
    
    # Let's verify if the lines match
    # Sometimes there's a mismatch if the file changed, but this is the exact diff applied to the current file!
    original_lines[start_idx:start_idx + len(new_lines_in_chunk)] = old_lines_in_chunk

with open("main.mm", "w") as f:
    f.write("\n".join(original_lines) + "\n")
print("Done reversing diff")
