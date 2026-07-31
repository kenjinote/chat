import socket, json, time, redis, threading

host = 'chat-lb-2026.japaneast.cloudapp.azure.com'
port = 4000

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(5.0)
s.connect((host, port))
s.sendall(json.dumps({'type': 'get_rooms'}).encode('utf-8') + b'\n')

rooms = ['ロビー']
start = time.time()
buffer = ""
while time.time() - start < 5:
    try:
        data = s.recv(4096).decode('utf-8')
        if not data: break
        buffer += data
        while '\n' in buffer:
            line, buffer = buffer.split('\n', 1)
            if not line.strip(): continue
            msg = json.loads(line)
            if msg.get('type') == 'room_list':
                for r in msg.get('rooms', []):
                    name = r.get('name')
                    if name and name not in rooms:
                        rooms.append(name)
                break
    except:
        pass
    if len(rooms) > 1: break
s.close()

print("Searching rooms:", rooms)

r = redis.Redis(host='your-redis-host.example.com', port=6379, db=0)
found = False

def search_room(room_name):
    global found
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(20.0)
        s.connect((host, port))
        
        join_msg = json.dumps({'type': 'join', 'secretId': f'bot_{room_name}', 'userName': f'bot_{room_name}', 'roomName': room_name, 'pub': True, 'nclr': 0}) + '\n'
        s.sendall(join_msg.encode('utf-8'))
        
        buffer = ""
        st = time.time()
        while time.time() - st < 20:
            if found: break
            data = s.recv(4096).decode('utf-8')
            if not data: break
            buffer += data
            while '\n' in buffer:
                line, buffer = buffer.split('\n', 1)
                if not line.strip(): continue
                try:
                    msg = json.loads(line)
                    # Redirect handler
                    if msg.get('type') == 'redirect':
                        new_host = msg.get('ip')
                        new_port = msg.get('port')
                        new_room = msg.get('roomName')
                        s.close()
                        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                        s.settimeout(20.0)
                        s.connect((new_host, new_port))
                        join_msg = json.dumps({'type': 'join', 'secretId': f'bot_{new_room}', 'userName': f'bot_{new_room}', 'roomName': new_room, 'pub': True, 'nclr': 0}) + '\n'
                        s.sendall(join_msg.encode('utf-8'))
                        continue
                        
                    if msg.get('type') == 'users':
                        for u in msg.get('users', []):
                            if u.get('userName') == 'User_15':
                                uid = u.get('userId')
                                r.set(f'banned_user:{uid}', '1', ex=86400)
                                print(f"BANNED User_15 in room {room_name}! ID: {uid}")
                                found = True
                                return
                    if (msg.get('type') == 'join' or msg.get('type') == 'leave') and msg.get('userName') == 'User_15':
                        uid = msg.get('userId')
                        r.set(f'banned_user:{uid}', '1', ex=86400)
                        print(f"BANNED User_15 in room {room_name}! ID: {uid}")
                        found = True
                        return
                except:
                    pass
        s.close()
    except Exception as e:
        print(f"Error in {room_name}: {e}")

threads = []
for room in rooms:
    t = threading.Thread(target=search_room, args=(room,))
    t.start()
    threads.append(t)

for t in threads:
    t.join()

if not found:
    print("Could not find User_15 in any public rooms.")
