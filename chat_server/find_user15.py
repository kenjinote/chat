import socket
import json
import time
import redis

def find_and_ban():
    host = 'chat-lb-2026.japaneast.cloudapp.azure.com'
    port = 4000
    r = redis.Redis(host='your-redis-host.example.com', port=6379, db=0)

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host, port))
    
    # join lobby
    join_msg = json.dumps({'type': 'join', 'secretId': 'admin-bot', 'userName': 'admin-bot', 'roomName': 'ロビー', 'pub': True, 'nclr': 0}) + '\n'
    s.sendall(join_msg.encode('utf-8'))
    
    print("Listening for User_15...")
    s.settimeout(10.0)
    
    start_time = time.time()
    buffer = ""
    while time.time() - start_time < 30:
        try:
            data = s.recv(4096).decode('utf-8')
            if not data:
                break
            buffer += data
            while '\n' in buffer:
                line, buffer = buffer.split('\n', 1)
                if not line.strip(): continue
                msg = json.loads(line)
                
                # Check users list
                if msg.get('type') == 'users':
                    for u in msg.get('users', []):
                        if u.get('userName') == 'User_15':
                            uid = u.get('userId')
                            print(f"Found User_15 in users list! ID: {uid}")
                            r.set(f'banned_user:{uid}', '1', ex=86400)
                            print("Banned.")
                            return
                
                # Check join messages
                if msg.get('type') == 'join' and msg.get('userName') == 'User_15':
                    uid = msg.get('userId')
                    print(f"Found User_15 in join msg! ID: {uid}")
                    r.set(f'banned_user:{uid}', '1', ex=86400)
                    print("Banned.")
                    return
        except Exception as e:
            print("Error:", e)
            break
            
    s.close()
    print("Could not find User_15 within 30 seconds.")

find_and_ban()
