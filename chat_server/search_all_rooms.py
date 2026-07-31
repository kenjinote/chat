import socket, json, time

host = 'chat-lb-2026.japaneast.cloudapp.azure.com'
port = 4000

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(5.0)
s.connect((host, port))

s.sendall(json.dumps({'type': 'get_rooms'}).encode('utf-8') + b'\n')

rooms = []
start = time.time()
buffer = ""
while time.time() - start < 5:
    try:
        data = s.recv(4096).decode('utf-8')
        if not data: break
        buffer += data
        while '\n' in buffer:
            line, buffer = buffer.split('\n', 1)
            msg = json.loads(line)
            if msg.get('type') == 'room_list':
                for r in msg.get('rooms', []):
                    rooms.append(r.get('roomName'))
                break
    except Exception as e:
        pass
    if rooms: break

s.close()

print("Found rooms:", rooms)

import redis, threading
r = redis.Redis(host='your-redis-host.example.com', port=6379, db=0)

found = False

def search_room(room_name):
    global found
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(10.0)
        s.connect((host, port))
        
        join_msg = json.dumps({'type': 'join', 'secretId': f'bot_{room_name}', 'userName': f'bot_{room_name}', 'roomName': room_name, 'pub': True, 'nclr': 0}) + '\n'
        s.sendall(join_msg.encode('utf-8'))
        
        buffer = ""
        st = time.time()
        while time.time() - st < 10:
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
                                print(f"BANNED User_15 in room {room_name}! ID: {uid}")
                                found = True
                                return
                    if msg.get('type') == 'join' and msg.get('userName') == 'User_15':
                        uid = msg.get('userId')
                        r.set(f'banned_user:{uid}', '1', ex=86400)
                        print(f"BANNED User_15 in room {room_name}! ID: {uid}")
                        found = True
                        return
                except:
                    pass
        s.close()
    except Exception as e:
        pass

threads = []
for room in rooms:
    t = threading.Thread(target=search_room, args=(room,))
    t.start()
    threads.append(t)

for t in threads:
    t.join()

if not found:
    print("Could not find User_15 in any public rooms.")
