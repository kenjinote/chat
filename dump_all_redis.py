import socket

def search_all():
    host = "your-redis-host.example.com"
    port = 6379
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    
    # Auth
    cmd_auth = "*2\r\n$4\r\nAUTH\r\n$19\r\nREDACTED_REDIS_PASSWORD\r\n"
    sock.sendall(cmd_auth.encode('utf-8'))
    sock.recv(4096)
    
    # KEYS *
    cmd = "*2\r\n$4\r\nKEYS\r\n$1\r\n*\r\n"
    sock.sendall(cmd.encode('utf-8'))
    
    resp = sock.recv(65536).decode('utf-8', errors='ignore')
    
    lines = resp.split("\r\n")
    keys = []
    for i in range(1, len(lines)):
        if lines[i] and not lines[i].startswith("$") and not lines[i].startswith("*"):
            keys.append(lines[i])
            
    for k in keys:
        if k.startswith("ip_of:") or k.startswith("room_owner:") or k.startswith("server_load:") or k.startswith("room_count:"):
            continue # We know these don't have usernames
            
        cmd2 = f"*2\r\n$3\r\nGET\r\n${len(k.encode('utf-8'))}\r\n{k}\r\n"
        sock.sendall(cmd2.encode('utf-8'))
        val_resp = sock.recv(4096).decode('utf-8', errors='ignore')
        if "discord" in val_resp.lower() or "5t7rut" in val_resp.lower():
            print(f"Found in key: {k} -> {val_resp}")
            
search_all()
