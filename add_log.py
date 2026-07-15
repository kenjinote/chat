import re

with open('H264Helper.cpp', 'r', encoding='utf-8') as f:
    code = f.read()

# Add a file logger
logger_code = """
#include <fstream>
void LogDecode(const char* msg, HRESULT hr) {
    static std::ofstream ofs("decode.log", std::ios::app);
    ofs << GetTickCount() << " " << msg << " hr=" << std::hex << hr << std::dec << std::endl;
}
"""

if "LogDecode" not in code:
    code = code.replace('#include "H264Helper.h"', '#include "H264Helper.h"\n' + logger_code)

# Inside DecodeFrame
decode_func = r"(bool H264Decoder::DecodeFrame\(const std::vector<uint8_t>& inH264, std::vector<uint8_t>& outRGB\) \{)"
code = re.sub(decode_func, r"\1\n    LogDecode(\"DecodeFrame start, size=\", inH264.size());", code)

process_input = r"hr = pDecoder->ProcessInput\(0, pInputSample, 0\);"
code = re.sub(process_input, r"hr = pDecoder->ProcessInput(0, pInputSample, 0);\n    LogDecode(\"ProcessInput\", hr);", code)

process_output = r"hr = pDecoder->ProcessOutput\(0, 1, &outputDataBuffer, &status\);"
code = re.sub(process_output, r"hr = pDecoder->ProcessOutput(0, 1, &outputDataBuffer, &status);\n            LogDecode(\"ProcessOutput\", hr);", code)

return_stmt = r"return !outRGB.empty\(\);"
code = re.sub(return_stmt, r"LogDecode(\"DecodeFrame end, returning\", !outRGB.empty());\n    return !outRGB.empty();", code)

with open('H264Helper.cpp', 'w', encoding='utf-8') as f:
    f.write(code)

print("Done")
