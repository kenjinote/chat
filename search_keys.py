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
    
    resp = sock.recv(4096).decode('utf-8', errors='ignore')
    
    lines = resp.split("\r\n")
    for line in lines:
        if "discord" in line.lower() or "5t7rut" in line.lower():
            print("Found interesting key:", line)
            
search_keys()
