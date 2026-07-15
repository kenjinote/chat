import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    code = f.read()

# 1. Include H264Helper.h and add globals
include_idx = code.find('#include "json11.hpp"')
if include_idx != -1:
    code = code[:include_idx] + '#include "H264Helper.h"\n' + code[include_idx:]

globals_insert = """
bool g_useH264 = true;
H264Encoder* g_pH264Encoder = nullptr;
H264Decoder* g_pH264Decoder = nullptr;
"""
global_idx = code.find('bool g_cameraActive = false;')
if global_idx != -1:
    code = code[:global_idx] + globals_insert + code[global_idx:]

# 2. Modify WebcamThread
webcam_encode_orig = """                std::vector<uint8_t> outJpeg;
                if (EncodeJPEG(g_WebcamWidth, g_WebcamHeight, pData, cbData, outJpeg)) {"""

webcam_encode_new = """                if (g_useH264) {
                    if (!g_pH264Encoder) {
                        g_pH264Encoder = new H264Encoder();
                        g_pH264Encoder->Initialize(g_WebcamWidth, g_WebcamHeight);
                    }
                    std::vector<uint8_t> h264;
                    if (g_pH264Encoder->EncodeFrame(pData, cbData, h264)) {
                        MediaHeader hdr;
                        hdr.sessionId = g_sessionId;
                        hdr.seqNum = g_VideoSeqNum++;
                        hdr.payloadType = 3; // H264
                        hdr.flags = 0;
                        hdr.timestamp = GetTickCount();
                        wcsncpy_s(hdr.userId, g_secretId.c_str(), 39);
                        
                        size_t offset = 0;
                        while (offset < h264.size()) {
                            size_t chunk = h264.size() - offset;
                            if (chunk > 1024) chunk = 1024;
                            
                            char udpPkt[1500];
                            memcpy(udpPkt, &hdr, sizeof(MediaHeader));
                            memcpy(udpPkt + sizeof(MediaHeader), h264.data() + offset, chunk);
                            
                            if (offset + chunk >= h264.size()) {
                                ((MediaHeader*)udpPkt)->flags |= 1; // EOF
                            }
                            
                            int sent = sendto(g_sock, udpPkt, sizeof(MediaHeader) + (int)chunk, 0, (sockaddr*)&serverUdp, sizeof(serverUdp));
                            if (sent == SOCKET_ERROR) {
                            }
                            
                            offset += chunk;
                        }
                    }
                } else {
                std::vector<uint8_t> outJpeg;
                if (EncodeJPEG(g_WebcamWidth, g_WebcamHeight, pData, cbData, outJpeg)) {"""

code = code.replace(webcam_encode_orig, webcam_encode_new)

# Close the else block after EncodeJPEG ends
webcam_encode_end_orig = """                            offset += chunk;
                        }
                    }
                }"""

webcam_encode_end_new = """                            offset += chunk;
                        }
                    }
                }
                }"""
code = code.replace(webcam_encode_end_orig, webcam_encode_end_new)


# 3. Modify RecvThread
recv_video_orig = """            } else if (pHdr->payloadType == 2) {
                // Video Data (MJPEG)"""
                
recv_video_new = """            } else if (pHdr->payloadType == 2 || pHdr->payloadType == 3) {
                // Video Data"""
code = code.replace(recv_video_orig, recv_video_new)

recv_decode_orig = """                    if (DecodeJPEG(completeData, pUser->lastVideoFrame, pUser->videoWidth, pUser->videoHeight)) {
                        pUser->lastVideoTime = GetTickCount();
                        InvalidateRect(g_hwndVideo, NULL, FALSE);
                    }"""

recv_decode_new = """                    if (payloadTypeCopy == 3) {
                        if (!g_pH264Decoder) {
                            g_pH264Decoder = new H264Decoder();
                            g_pH264Decoder->Initialize(640, 480);
                        }
                        std::vector<uint8_t> h264(completeData.begin(), completeData.end());
                        std::vector<uint8_t> rgbOut;
                        if (g_pH264Decoder->DecodeFrame(h264, rgbOut)) {
                            pUser->lastVideoFrame = rgbOut;
                            pUser->videoWidth = 640;
                            pUser->videoHeight = 480;
                            pUser->lastVideoTime = GetTickCount();
                            InvalidateRect(g_hwndVideo, NULL, FALSE);
                        }
                    } else {
                        if (DecodeJPEG(completeData, pUser->lastVideoFrame, pUser->videoWidth, pUser->videoHeight)) {
                            pUser->lastVideoTime = GetTickCount();
                            InvalidateRect(g_hwndVideo, NULL, FALSE);
                        }
                    }"""

# Wait, `payloadTypeCopy` is not defined? Let's check RecvThread.
# Let's replace the whole block by extracting payloadType inside the Reassembly condition.
# In RecvThread:
# if (pHdr->flags & 1) { // EOF
#     std::vector<uint8_t> completeData;
#     ...
#     if (DecodeJPEG(...)) { ... }
# }
# To get the payloadType inside EOF block, we can just use the current packet's pHdr->payloadType.

recv_decode_orig_2 = """                    if (DecodeJPEG(completeData, pUser->lastVideoFrame, pUser->videoWidth, pUser->videoHeight)) {
                        pUser->lastVideoTime = GetTickCount();
                        InvalidateRect(g_hwndVideo, NULL, FALSE);
                    }"""

recv_decode_new_2 = """                    if (pHdr->payloadType == 3) {
                        if (!g_pH264Decoder) {
                            g_pH264Decoder = new H264Decoder();
                            g_pH264Decoder->Initialize(640, 480);
                        }
                        std::vector<uint8_t> h264(completeData.begin(), completeData.end());
                        std::vector<uint8_t> rgbOut;
                        if (g_pH264Decoder->DecodeFrame(h264, rgbOut)) {
                            pUser->lastVideoFrame = rgbOut;
                            pUser->videoWidth = 640;
                            pUser->videoHeight = 480;
                            pUser->lastVideoTime = GetTickCount();
                            InvalidateRect(g_hwndVideo, NULL, FALSE);
                        }
                    } else {
                        if (DecodeJPEG(completeData, pUser->lastVideoFrame, pUser->videoWidth, pUser->videoHeight)) {
                            pUser->lastVideoTime = GetTickCount();
                            InvalidateRect(g_hwndVideo, NULL, FALSE);
                        }
                    }"""
code = code.replace(recv_decode_orig_2, recv_decode_new_2)


# 4. Modify Context Menu
menu_orig = """            AppendMenuW(hMenu, MF_STRING, 1002, L"ミュート / ミュート解除");"""
menu_new = """            AppendMenuW(hMenu, MF_STRING, 1002, L"ミュート / ミュート解除");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, g_useH264 ? (MF_STRING | MF_CHECKED) : MF_STRING, 1003, L"H.264 ハードウェアエンコード");"""
code = code.replace(menu_orig, menu_new)

menu_cmd_orig = """            case 1002: {
                // mute toggle handled later maybe or global?"""
menu_cmd_new = """            case 1003: {
                g_useH264 = !g_useH264;
                if (!g_useH264 && g_pH264Encoder) {
                    delete g_pH264Encoder;
                    g_pH264Encoder = nullptr;
                }
                break;
            }
            case 1002: {"""
code = code.replace(menu_cmd_orig, menu_cmd_new)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(code)

print("Source.cpp modified successfully")
