with open("diff_output.txt", "r") as f:
    text = f.read()

blocks = text.split("FOUND-------------------------")
last_block = blocks[-1]
diff_start = last_block.find("[diff_block_start]")
diff_end = last_block.find("[diff_block_end]")
if diff_end == -1: diff_end = len(last_block)

diff_text = last_block[diff_start + len("[diff_block_start]"):diff_end]
diff_lines = diff_text.strip().splitlines()
with open("main.patch", "w") as out:
    out.write("--- main.mm\n")
    out.write("+++ main.mm\n")
    for line in diff_lines:
        out.write(line + "\n")
