import re

with open("diff_output.txt", "r") as f:
    content = f.read()

blocks = content.split("FOUND-------------------------")
if len(blocks) > 1:
    last_block = blocks[-1]
    diff_start = last_block.find("[diff_block_start]")
    diff_end = last_block.find("[diff_block_end]")
    if diff_start != -1 and diff_end != -1:
        diff_content = last_block[diff_start + len("[diff_block_start]"):diff_end].strip()
        
        with open("main.patch", "w") as out:
            # Add proper diff headers so patch command accepts it
            out.write("--- main.mm\n")
            out.write("+++ main.mm\n")
            out.write(diff_content + "\n")
