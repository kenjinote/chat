import socket

def get_ip(username):
    host = "your-redis-host.example.com"
    port = 6379
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    
    # Auth
    cmd_auth = "*2\r\n$4\r\nAUTH\r\n$19\r\nREDACTED_REDIS_PASSWORD\r\n"
    sock.sendall(cmd_auth.encode('utf-8'))
    auth_resp = sock.recv(4096).decode('utf-8')
    print("Auth resp:", repr(auth_resp))
    
    # Get
    key = f"ip_of:{username}"
    cmd = f"*2\r\n$3\r\nGET\r\n${len(key.encode('utf-8'))}\r\n{key}\r\n"
    sock.sendall(cmd.encode('utf-8'))
    
    resp = sock.recv(4096).decode('utf-8')
    print("Raw response:", repr(resp))
    
    lines = resp.split("\r\n")
    if len(lines) >= 2 and not lines[0].startswith("-") and not lines[0].startswith("$-1"):
        return lines[1]
    return "Not found"

print(get_ip("https://discord.gg/5t7RUt69Yhttps://discord.gg/5t7"))
