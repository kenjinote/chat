import socket

def get_ips():
    host = "your-redis-host.example.com"
    port = 6379
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    
    # Auth
    cmd_auth = "*2\r\n$4\r\nAUTH\r\n$19\r\nREDACTED_REDIS_PASSWORD\r\n"
    sock.sendall(cmd_auth.encode('utf-8'))
    sock.recv(4096)
    
    uuids = ['49aa50ff-7c33-3618-4e7a-04d374c72518','c0b789d6-1b2f-01f1-e632-682f030f67f7','ca100fe2-9190-57c3-f447-6edfebcfc40f']
    for u in uuids:
        k = f"ip_of:{u}"
        c = f"*2\r\n$3\r\nGET\r\n${len(k.encode('utf-8'))}\r\n{k}\r\n"
        sock.sendall(c.encode('utf-8'))
        r = sock.recv(4096).decode('utf-8')
        print(f"{u}: {repr(r)}")

get_ips()
