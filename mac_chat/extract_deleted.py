import re

with open("diff_output.txt", "r") as f:
    lines = f.readlines()

deleted_lines = []
for line in lines:
    if line.startswith("-") and not line.startswith("---"):
        deleted_lines.append(line[1:])

with open("deleted_lines.m", "w") as f:
    f.writelines(deleted_lines)
