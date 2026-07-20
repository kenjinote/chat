import os

file_path = 'Source.cpp'

with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

target = """                if (g_isServerUdpAddrResolved) {
                    serverUdp.sin_addr = g_serverUdpAddr;
                    sendto(g_sock, (char*)&hdr, sizeof(hdr), 0, (sockaddr*)&serverUdp, sizeof(serverUdp));
                }"""

replacement = """                if (g_isServerUdpAddrResolved) {
                    serverUdp.sin_addr = g_serverUdpAddr;
                    int sent = sendto(g_sock, (char*)&hdr, sizeof(hdr), 0, (sockaddr*)&serverUdp, sizeof(serverUdp));
                    LogToFile("UDP KEEPALIVE SENT: sent=" + std::to_string(sent) + " to IP=" + std::string(inet_ntoa(g_serverUdpAddr)));
                }"""

if target in content:
    content = content.replace(target, replacement)
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Log added")
else:
    print("Target not found")
