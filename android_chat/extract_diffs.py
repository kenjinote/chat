import json
import os
import subprocess

log_file = r'C:\Users\kenjinote\.gemini\antigravity\brain\50a60d6e-2c23-49a3-8e11-7463eb158313\.system_generated\logs\transcript_full.jsonl'
target_file = 'app/src/main/java/com/example/chat/MainActivity.kt'

# Restore to July 4 base
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
        
        # Stop at step 3849 (we want the state BEFORE the change at 3849)
        # Actually, let's stop at 3799 to be safe. The user said around 22:00.
        # Step 3799 is 12:56Z, which is 21:56 JST.
        if step > 3800:
            break
            
        if data.get('type') == 'TOOL_RESPONSE':
            content = data.get('content', '')
            if '[diff_block_start]' in content and target_file in content:
                # Extract diff
                start_idx = content.find('[diff_block_start]') + len('[diff_block_start]')
                end_idx = content.find('[diff_block_end]')
                diff_body = content[start_idx:end_idx].strip()
                
                # Prepend headers
                full_diff = f"--- {target_file}\n+++ {target_file}\n{diff_body}\n"
                
                with open('temp.patch', 'w', encoding='utf-8') as pf:
                    pf.write(full_diff)
                
                res = subprocess.run([patch_exe, '-p0', '--no-backup-if-mismatch', '-i', 'temp.patch'], capture_output=True, text=True)
                if res.returncode == 0:
                    diffs_applied += 1
                else:
                    print(f"Failed to apply patch at step around {step}. Return code: {res.returncode}")
                    print(res.stdout)
                    print(res.stderr)

print(f"Successfully applied {diffs_applied} diffs!")
