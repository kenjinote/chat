import socket

def search_keys():
    host = "your-redis-host.example.com"
    port = 6379
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    
    # Auth
    cmd_auth = "*2\r\n$4\r\nAUTH\r\n$19\r\nREDACTED_REDIS_PASSWORD\r\n"
    sock.sendall(cmd_auth.encode('utf-8'))
    auth_resp = sock.recv(4096).decode('utf-8')
    
    # KEYS
    cmd = "*2\r\n$4\r\nKEYS\r\n$7\r\nip_of:*\r\n"
    sock.sendall(cmd.encode('utf-8'))
    
    resp = sock.recv(4096).decode('utf-8')
    print("Raw response:", repr(resp))
    
    lines = resp.split("\r\n")
    keys = []
    for line in lines:
        if line.startswith("ip_of:"):
            keys.append(line)
            
    for k in keys:
        cmd2 = f"*2\r\n$3\r\nGET\r\n${len(k.encode('utf-8'))}\r\n{k}\r\n"
        sock.sendall(cmd2.encode('utf-8'))
        val_resp = sock.recv(4096).decode('utf-8')
        val_lines = val_resp.split("\r\n")
        if len(val_lines) >= 2 and not val_lines[0].startswith("-") and not val_lines[0].startswith("$-1"):
            print(f"{k} = {val_lines[1]}")

search_keys()
