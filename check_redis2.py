import socket
import sys

def redis_command(sock, *args):
    cmd = f"*{len(args)}\r\n"
    for arg in args:
        cmd += f"${len(arg)}\r\n{arg}\r\n"
    sock.sendall(cmd.encode('utf-8'))
    
    # Very simple parsing
    resp = b""
    while True:
        chunk = sock.recv(4096)
        resp += chunk
        if len(chunk) < 4096:
            break
    return resp

try:
    host = "your-redis-host.example.com"
    port = 6379
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    
    keys_resp = redis_command(sock, "KEYS", "*").decode('utf-8')
    print("Keys response:", keys_resp)
    
    # Manually parsing the *N response
    lines = keys_resp.split("\r\n")
    keys = []
    for line in lines:
        if line.startswith("room_count:"):
            keys.append(line)
            
    print("Found keys:", keys)
    total_users = 0
    for key in keys:
        val_resp = redis_command(sock, "GET", key).decode('utf-8')
        lines2 = val_resp.split("\r\n")
        # $length\r\nvalue\r\n
        if len(lines2) >= 2 and not lines2[0].startswith("-"):
            val = lines2[1]
            print(f"{key}: {val}")
            total_users += int(val)
            
    print(f"Total users: {total_users}")
    
except Exception as e:
    print("Error:", e)
