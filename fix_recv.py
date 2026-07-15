import re

with open('Source.cpp', 'r', encoding='utf-8') as f:
    code = f.read()

# 1. Change payloadType from 3 to 5 in WebcamThread for H264
webcam_orig = """                        mhdr->payloadType = 3; 
                        mhdr->flags = (i == 0 ? 1 : 0) | (i == totalFragments - 1 ? 2 : 0);"""
webcam_new = """                        mhdr->payloadType = 5; 
                        mhdr->flags = (i == 0 ? 1 : 0) | (i == totalFragments - 1 ? 2 : 0);"""
code = code.replace(webcam_orig, webcam_new)

# 2. Update DecodeTaskData and DecodeTaskProc
decode_orig = """struct DecodeTaskData {
    std::vector<uint8_t> jpegData;
    std::wstring username;
};

DWORD WINAPI DecodeTaskProc(LPVOID lpParam) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    DecodeTaskData* data = (DecodeTaskData*)lpParam;
    DecodeJPEG(data->jpegData, data->username);
    delete data;
    if (SUCCEEDED(hr)) {
        CoUninitialize();
    }
    return 0;
}"""

decode_new = """struct DecodeTaskData {
    std::vector<uint8_t> mediaData;
    std::wstring username;
    bool isH264;
};

DWORD WINAPI DecodeTaskProc(LPVOID lpParam) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    DecodeTaskData* data = (DecodeTaskData*)lpParam;
    if (data->isH264) {
        if (!g_pH264Decoder) {
            g_pH264Decoder = new H264Decoder();
            g_pH264Decoder->Initialize(640, 480);
        }
        std::vector<uint8_t> rgbOut;
        if (g_pH264Decoder->DecodeFrame(data->mediaData, rgbOut)) {
            std::lock_guard<std::mutex> lock(g_videoMutex);
            g_videoFrames[data->username] = rgbOut;
            g_videoDimensions[data->username] = {640, 480};
        }
    } else {
        DecodeJPEG(data->mediaData, data->username);
    }
    delete data;
    if (SUCCEEDED(hr)) {
        CoUninitialize();
    }
    return 0;
}"""
code = code.replace(decode_orig, decode_new)

# 3. Update RecvThread
recv_orig = """                        else if (mhdr->payloadType == 2) {
                            std::wstring wname = mhdr->userId;
                            uint16_t seq = mhdr->seqNum;
                            int fragIdx = mhdr->timestamp;
                            int len = res - sizeof(MediaHeader);"""

recv_new = """                        else if (mhdr->payloadType == 2 || mhdr->payloadType == 5) {
                            std::wstring wname = mhdr->userId;
                            uint16_t seq = mhdr->seqNum;
                            int fragIdx = mhdr->timestamp;
                            int len = res - sizeof(MediaHeader);
                            bool isH264 = (mhdr->payloadType == 5);"""
code = code.replace(recv_orig, recv_new)

recv_queue_orig = """                            if (vbuf.totalFragments > 0 && vbuf.fragments.size() == vbuf.totalFragments) {
                                DecodeTaskData* taskData = new DecodeTaskData();
                                taskData->username = wname;
                                for (int i = 0; i < vbuf.totalFragments; i++) {
                                    taskData->jpegData.insert(taskData->jpegData.end(), vbuf.fragments[i].begin(), vbuf.fragments[i].end());
                                }
                                QueueUserWorkItem(DecodeTaskProc, taskData, WT_EXECUTEDEFAULT);
                                g_videoBuffers[wname].erase(seq);
                            }"""

recv_queue_new = """                            if (vbuf.totalFragments > 0 && vbuf.fragments.size() == vbuf.totalFragments) {
                                DecodeTaskData* taskData = new DecodeTaskData();
                                taskData->username = wname;
                                taskData->isH264 = isH264;
                                for (int i = 0; i < vbuf.totalFragments; i++) {
                                    taskData->mediaData.insert(taskData->mediaData.end(), vbuf.fragments[i].begin(), vbuf.fragments[i].end());
                                }
                                QueueUserWorkItem(DecodeTaskProc, taskData, WT_EXECUTEDEFAULT);
                                g_videoBuffers[wname].erase(seq);
                            }"""
code = code.replace(recv_queue_orig, recv_queue_new)

with open('Source.cpp', 'w', encoding='utf-8') as f:
    f.write(code)

print("Source.cpp fixed for H264 Recv and payloadType 5")
