import socket

def check_reports():
    host = "your-redis-host.example.com"
    port = 6379
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    
    # Auth
    cmd_auth = "*2\r\n$4\r\nAUTH\r\n$19\r\nREDACTED_REDIS_PASSWORD\r\n"
    sock.sendall(cmd_auth.encode('utf-8'))
    sock.recv(4096)
    
    # KEYS report:*
    cmd = "*2\r\n$4\r\nKEYS\r\n$8\r\nreport:*\r\n"
    sock.sendall(cmd.encode('utf-8'))
    resp = sock.recv(4096).decode('utf-8', errors='ignore')
    print("report:* keys:", resp)

    # KEYS report_ip:*
    cmd = "*2\r\n$4\r\nKEYS\r\n$11\r\nreport_ip:*\r\n"
    sock.sendall(cmd.encode('utf-8'))
    resp = sock.recv(4096).decode('utf-8', errors='ignore')
    print("report_ip:* keys:", resp)

check_reports()
