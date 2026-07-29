import socket
import json
import sys

servers = [
    "chat-vm-a-2026.japaneast.cloudapp.azure.com", "chat-vm-b-2026.japaneast.cloudapp.azure.com",
    "chat-vm-c-2026.japaneast.cloudapp.azure.com", "chat-vm-d-2026.japaneast.cloudapp.azure.com",
    "chat-vm-e-2026.japaneast.cloudapp.azure.com", "chat-vm-f-2026.japaneast.cloudapp.azure.com",
    "chat-vm-g-2026.japaneast.cloudapp.azure.com", "chat-vm-h-2026.japaneast.cloudapp.azure.com",
    "chat-vm-i-2026.japaneast.cloudapp.azure.com", "chat-vm-j-2026.japaneast.cloudapp.azure.com",
    "chat-vm-k-2026.japaneast.cloudapp.azure.com", "chat-vm-l-2026.japaneast.cloudapp.azure.com",
    "chat-vm-m-2026.japaneast.cloudapp.azure.com", "chat-vm-n-2026.japaneast.cloudapp.azure.com",
    "chat-vm-o-2026.japaneast.cloudapp.azure.com", "chat-vm-p-2026.japaneast.cloudapp.azure.com",
    "chat-vm-q-2026.japaneast.cloudapp.azure.com", "chat-vm-r-2026.japaneast.cloudapp.azure.com",
    "chat-vm-s-2026.japaneast.cloudapp.azure.com", "chat-vm-t-2026.japaneast.cloudapp.azure.com",
    "chat-vm-u-2026.japaneast.cloudapp.azure.com", "chat-vm-v-2026.japaneast.cloudapp.azure.com",
    "chat-vm-w-2026.japaneast.cloudapp.azure.com", "chat-vm-x-2026.japaneast.cloudapp.azure.com",
    "chat-vm-y-2026.japaneast.cloudapp.azure.com", "chat-vm-z-2026.japaneast.cloudapp.azure.com"
]

room_names = ["ロビー", "Lobby", "Public"]
found_users = set()

for srv in servers:
    for rname in room_names:
        try:
            s = socket.create_connection((srv, 4000), timeout=1.0)
            join_msg = json.dumps({
                "type": "join",
                "userId": "probe_bot",
                "userName": "probe_bot",
                "roomName": rname,
                "pub": True,
                "nclr": 0
            }) + "\n"
            s.sendall(join_msg.encode('utf-8'))
            
            f = s.makefile()
            for _ in range(5):
                line = f.readline()
                if not line: break
                msg = json.loads(line)
                if msg.get("type") == "redirect":
                    break
                if msg.get("type") == "users":
                    for u in msg.get("users", []):
                        uid = u.get("userId")
                        uname = u.get("userName")
                        if uid != "probe_bot":
                            found_users.add(f"{uname} (ID: {uid}) on {srv} in {rname}")
                    break
            s.close()
        except Exception as e:
            pass

print("Connected users:")
for u in found_users:
    print(u)
