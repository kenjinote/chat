import re

with open("deleted_lines.m", "r") as f:
    lines = f.readlines()

def print_method(method_name):
    print("=== " + method_name + " ===")
    in_method = False
    for line in lines:
        if line.startswith("- (void)" + method_name) or line.startswith("- (BOOL)" + method_name):
            in_method = True
        if in_method:
            print(line, end="")
            if line.startswith("}"):
                in_method = False
                print()

print_method("applicationDidFinishLaunching")
print_method("layoutControls")
print_method("onSendClicked")
print_method("onFormatChanged")
print_method("onVoiceClicked")
print_method("onVideoClicked")
