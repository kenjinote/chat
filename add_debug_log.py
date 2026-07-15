import os

with open('H264Helper.cpp', 'r', encoding='utf-8') as f:
    code = f.read()

log_header = """#include <fstream>
static void LogMsg(const std::string& msg) {
    static std::ofstream ofs("h264_debug.log", std::ios::app);
    ofs << GetTickCount() << " " << msg << std::endl;
}
"""
code = code.replace('#include "H264Helper.h"', '#include "H264Helper.h"\n' + log_header)

code = code.replace("bool H264Decoder::DecodeFrame(const std::vector<uint8_t>& inH264, std::vector<uint8_t>& outRgb32) {",
                    "bool H264Decoder::DecodeFrame(const std::vector<uint8_t>& inH264, std::vector<uint8_t>& outRgb32) {\n    LogMsg(\"DecodeFrame Start size=\" + std::to_string(inH264.size()));")

code = code.replace("HRESULT hr = pDecoder->ProcessInput(0, pSample, 0);",
                    "HRESULT hr = pDecoder->ProcessInput(0, pSample, 0);\n    if (FAILED(hr)) LogMsg(\"ProcessInput Failed hr=\" + std::to_string(hr));")

code = code.replace("if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {",
                    "if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {\n            //LogMsg(\"Dec NEED_MORE_INPUT\");")

code = code.replace("if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {",
                    "if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {\n            LogMsg(\"Dec STREAM_CHANGE\");")

code = code.replace("hr = pConverter->ProcessInput(0, outputDataBuffer.pSample, 0);",
                    "hr = pConverter->ProcessInput(0, outputDataBuffer.pSample, 0);\n            if (FAILED(hr)) LogMsg(\"Conv ProcessInput Failed hr=\" + std::to_string(hr));")

code = code.replace("return !outRgb32.empty();",
                    "if (outRgb32.empty()) LogMsg(\"DecodeFrame End: EMPTY\");\n    else LogMsg(\"DecodeFrame End: SUCCESS\");\n    return !outRgb32.empty();")

with open('H264Helper.cpp', 'w', encoding='utf-8') as f:
    f.write(code)
print("Done")
