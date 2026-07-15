import re

with open('H264Helper.cpp', 'r') as f:
    content = f.read()

# The replace_file_content duplicated a block. Let's fix it by regex
# We want to remove everything between the first `return !outH264.empty();\n}` and `H264Decoder::H264Decoder() {}`

pattern = r'(return !outH264\.empty\(\);\n\})(.*?)(H264Decoder::H264Decoder\(\) \{\})'
content = re.sub(pattern, r'\1\n\n\3', content, flags=re.DOTALL)

with open('H264Helper.cpp', 'w') as f:
    f.write(content)
