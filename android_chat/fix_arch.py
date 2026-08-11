import re

path = 'app/src/main/java/com/example/chat/ChatViewModel.kt'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Remove from connect()
connect_audio_video = '''                networkClient.sendTcpMessage(joinMsg)

                audioEngine.initAudio()
                
                videoEngine.processEncoderOutput { h264Data ->
                    if (serverSessionId != 0) {
                        val header = MediaHeader(
                            sessionId = serverSessionId,
                            seqNum = videoSeqNum++,
                            payloadType = MediaHeader.VIDEO_H264,
                            flags = 0,
                            timestamp = (System.currentTimeMillis() and 0xFFFFFFFF).toInt(),
                            userId = myUserId
                        )
                        networkClient.sendUdpPacket(header.toByteArray(h264Data))
                    }
                }'''

if connect_audio_video in content:
    content = content.replace(connect_audio_video, '                networkClient.sendTcpMessage(joinMsg)')
else:
    print("WARNING: Could not find connect_audio_video block to remove.")

# 2. Find the Thread { ... receiveUdpPacket() } block in init
udp_thread_regex = re.compile(r'(\s*Thread\s*\{\s*while\s*\(true\)\s*\{\s*val udpData = networkClient\.receiveUdpPacket\(\) \?: break.*?\}\s*\.start\(\)\n)', re.DOTALL)
udp_match = udp_thread_regex.search(content)

if udp_match:
    udp_thread_block = udp_match.group(1)
    # Remove from its current location
    content = content.replace(udp_thread_block, '')
    
    # We will inject it inside connect(), specifically right after startListening()
    start_listening_line = '                startListening()\n'
    
    if start_listening_line in content:
        content = content.replace(start_listening_line, start_listening_line + udp_thread_block)
    else:
        print("WARNING: Could not find startListening() line to inject UDP thread.")
else:
    print("WARNING: Could not find UDP thread block.")


# 3. Add audioEngine.initAudio() and videoEngine.processEncoderOutput to init {}
init_audio_video_block = '''
        audioEngine.initAudio()
        
        videoEngine.processEncoderOutput { h264Data ->
            if (serverSessionId != 0) {
                val header = MediaHeader(
                    sessionId = serverSessionId,
                    seqNum = videoSeqNum++,
                    payloadType = MediaHeader.VIDEO_H264,
                    flags = 0,
                    timestamp = (System.currentTimeMillis() and 0xFFFFFFFF).toInt(),
                    userId = myUserId
                )
                networkClient.sendUdpPacket(header.toByteArray(h264Data))
            }
        }
'''

# Find the end of init {}
init_start_regex = re.compile(r'(\s*init\s*\{)')
match = init_start_regex.search(content)
if match:
    # Insert right after 'init {'
    insert_pos = match.end(1)
    content = content[:insert_pos] + init_audio_video_block + content[insert_pos:]
else:
    print("WARNING: Could not find init { } block.")

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Applied architectural fixes for initialization.")
