import socket, json, time, threading, redis

r = redis.Redis(host='your-redis-host.example.com', port=6379, db=0)

found = False

def check_conn(i):
    global found
    host = 'chat-lb-2026.japaneast.cloudapp.azure.com'
    port = 4000
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(15.0)
        s.connect((host, port))
        
        join_msg = json.dumps({'type': 'join', 'secretId': f'bot_{i}', 'userName': f'bot_{i}', 'roomName': 'ロビー', 'pub': True, 'nclr': 0}) + '\n'
        s.sendall(join_msg.encode('utf-8'))
        
        buffer = ""
        start = time.time()
        while time.time() - start < 15:
            if found: break
            data = s.recv(4096).decode('utf-8')
            if not data: break
            buffer += data
            while '\n' in buffer:
                line, buffer = buffer.split('\n', 1)
                if not line.strip(): continue
                try:
                    msg = json.loads(line)
                    if msg.get('type') == 'users':
                        for u in msg.get('users', []):
                            if u.get('userName') == 'User_15':
                                uid = u.get('userId')
                                r.set(f'banned_user:{uid}', '1', ex=86400)
                                print(f"BANNED via users list on bot {i}: {uid}")
                                found = True
                                return
                    if msg.get('type') == 'join' and msg.get('userName') == 'User_15':
                        uid = msg.get('userId')
                        r.set(f'banned_user:{uid}', '1', ex=86400)
                        print(f"BANNED via join msg on bot {i}: {uid}")
                        found = True
                        return
                except:
                    pass
        s.close()
    except Exception as e:
        pass

threads = []
for i in range(50):
    t = threading.Thread(target=check_conn, args=(i,))
    t.start()
    threads.append(t)

for t in threads:
    t.join()

print("Search complete.")
