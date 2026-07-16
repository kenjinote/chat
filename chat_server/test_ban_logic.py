import socket
import json
import time
import hashlib

def get_user_id(secret_id):
    return hashlib.sha256(secret_id.encode('utf-8')).hexdigest()

def test_ban_logic():
    host = 'chat-sfu-a-2026.japaneast.azurecontainer.io'
    port = 4000
    target_secret = "test_target_secret"
    target_id = get_user_id(target_secret)
    print(f"Testing against Target User ID: {target_id}")

    # 1. 7つの異なる端末（secretId）から同じターゲットを通報する
    for i in range(1, 8):
        reporter_secret = f"test_reporter_{i}"
        
        try:
            # Connect to server
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(2)
            s.connect((host, port))
            print(f"[{reporter_secret}] Connected")
            
            # Send Join message
            join_msg = {
                "type": "join",
                "secretId": reporter_secret,
                "userName": f"Reporter {i}",
                "roomName": "TestRoom",
                "nclr": 0
            }
            s.sendall((json.dumps(join_msg) + "\n").encode('utf-8'))
            time.sleep(0.5)
            
            # Send Report message
            report_msg = {
                "type": "report",
                "target": target_id
            }
            s.sendall((json.dumps(report_msg) + "\n").encode('utf-8'))
            print(f"[{reporter_secret}] Sent report for {target_id}")
            time.sleep(1)
            s.close()
            time.sleep(1)
        except Exception as e:
            print(f"Error for {reporter_secret}: {e}")

    print("Finished sending 7 reports from the same IP but different User IDs.")
    print("If the new logic is working, the target should NOT be banned because IP count is 1.")

if __name__ == "__main__":
    test_ban_logic()
