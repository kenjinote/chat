import socket
import json
s=socket.socket()
s.connect(('chat-lb-2026.japaneast.cloudapp.azure.com', 4000))
welcome=s.recv(4096).decode('utf-8')
print('welcome:', welcome)
s.send(b'{\"type\":\"get_rooms\"}\n')
print('rooms:', s.recv(4096).decode('utf-8'))
