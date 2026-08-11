import json
import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(5)
try:
    s.connect(('chat-vm-b-2026.japaneast.cloudapp.azure.com', 4000))
    join_msg = {
        "type": "join",
        "userName": "AndroidUser_Test_3",
        "secretId": "test-uuid-3",
        "pub": True,
        "roomName": "Public",
        "nclr": -1
    }
    
    payload = json.dumps(join_msg) + "\n"
    print(f"Sending: {payload.strip()}")
    s.send(payload.encode('utf-8'))
    
    for i in range(5):
        response = s.recv(4096).decode('utf-8')
        if not response:
            break
        print(f"Received: {response.strip()}")
        
except Exception as e:
    print(f"Error: {e}")
finally:
    s.close()
