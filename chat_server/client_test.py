import socket
import json
import time

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('chat-sfu-a-2026.japaneast.azurecontainer.io', 4000))
print('RECV1:', s.recv(4096).decode('utf-8'))

join_msg = json.dumps({'type': 'join', 'secretId': 'test-secret', 'userName': 'tester', 'roomName': 'Public', 'pub': True, 'nclr': 0}) + '\n'
s.sendall(join_msg.encode('utf-8'))

time.sleep(1)
print('RECV2:', s.recv(4096).decode('utf-8'))
s.close()
