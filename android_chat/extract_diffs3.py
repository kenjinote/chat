import json
import os
import subprocess

log_file = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'
target_file = 'app/src/main/java/com/example/chat/MainActivity.kt'

subprocess.run(['git', 'restore', target_file])

patch_exe = r'C:\Program Files\Git\usr\bin\patch.exe'

diffs_applied = 0
with open(log_file, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
        except:
            continue
            
        step = data.get('step_index', 0)
        
        # We want to stop at the change made BEFORE the user request at 3849.
        # But wait, CODE_ACTION steps might not have step_index!
        # Actually, CODE_ACTION has step_index.
        if step > 3800:
            break
            
        if data.get('type') == 'CODE_ACTION':
            content = data.get('content', '')
            if '[diff_block_start]' in content and 'MainActivity.kt' in content:
                start_idx = content.find('[diff_block_start]') + len('[diff_block_start]')
                end_idx = content.find('[diff_block_end]')
                diff_body = content[start_idx:end_idx].strip()
                
                full_diff = f"--- {target_file}\n+++ {target_file}\n{diff_body}\n"
                
                with open('temp.patch', 'w', encoding='utf-8') as pf:
                    pf.write(full_diff)
                
                res = subprocess.run([patch_exe, '-p0', '--no-backup-if-mismatch', '-i', 'temp.patch'], capture_output=True, text=True)
                if res.returncode == 0:
                    diffs_applied += 1
                else:
                    print(f"Failed to apply patch at step around {step}. Return code: {res.returncode}")
                    # don't print full output to avoid spam

print(f"Successfully applied {diffs_applied} diffs!")
