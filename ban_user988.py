import socket
import json
import time
import redis
import threading

found = False

def find_and_ban(i):
    global found
    host = 'chat-lb-2026.japaneast.cloudapp.azure.com'
    port = 4000
    r = redis.Redis(host='your-redis-host.example.com', port=6379, db=0, password="REDACTED_REDIS_PASSWORD")

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((host, port))
    except Exception as e:
        return
    
    # join lobby
    join_msg = json.dumps({'type': 'join', 'secretId': f'admin-bot-{i}', 'userName': f'admin-bot-{i}', 'roomName': 'ロビー', 'pub': True, 'nclr': 0}) + '\n'
    s.sendall(join_msg.encode('utf-8'))
    
    s.settimeout(10.0)
    
    start_time = time.time()
    buffer = ""
    while time.time() - start_time < 30:
        if found: break
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
                        if u.get('userName') == 'User_988':
                            uid = u.get('userId')
                            print(f"Found User_988 in users list! ID: {uid}")
                            r.set(f'banned_user:{uid}', '1', ex=86400)
                            print("Banned.")
                            found = True
                            return
                
                # Check join messages
                if msg.get('type') == 'join' and msg.get('userName') == 'User_988':
                    uid = msg.get('userId')
                    print(f"Found User_988 in join msg! ID: {uid}")
                    r.set(f'banned_user:{uid}', '1', ex=86400)
                    print("Banned.")
                    found = True
                    return
        except Exception as e:
            break
            
    s.close()

threads = []
for i in range(20):
    t = threading.Thread(target=find_and_ban, args=(i,))
    t.start()
    threads.append(t)

for t in threads:
    t.join()

if not found:
    print("Could not find User_988.")
