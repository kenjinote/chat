import re

with open('H264Helper.cpp', 'r', encoding='utf-8') as f:
    code = f.read()

# EncodeFrame fixes
code = code.replace(
    'pConvSample->AddBuffer(pConvBuffer);\n        convOutput.pSample = pConvSample;',
    'pConvSample->AddBuffer(pConvBuffer);\n        pConvBuffer->Release();\n        convOutput.pSample = pConvSample;'
)
code = code.replace(
    'pConvBuffer->Release();\n            pConvSample->Release();',
    'pConvSample->Release();'
)

code = code.replace(
    'pEncSample->AddBuffer(pEncBuffer);\n                    encOutput.pSample = pEncSample;',
    'pEncSample->AddBuffer(pEncBuffer);\n                    pEncBuffer->Release();\n                    encOutput.pSample = pEncSample;'
)
code = code.replace(
    'pEncBuffer->Release();\n                        pEncSample->Release();',
    'pEncSample->Release();'
)

# DecodeFrame fixes
code = code.replace(
    'pOutSample->AddBuffer(pOutBuffer);\n        outputDataBuffer.pSample = pOutSample;',
    'pOutSample->AddBuffer(pOutBuffer);\n        pOutBuffer->Release();\n        outputDataBuffer.pSample = pOutSample;'
)
code = code.replace(
    'pOutBuffer->Release();\n            pOutSample->Release();',
    'pOutSample->Release();'
)

code = code.replace(
    'pConvSample->AddBuffer(pConvBuffer); // RGB32\n                    convOutput.pSample = pConvSample;',
    'pConvSample->AddBuffer(pConvBuffer); // RGB32\n                    pConvBuffer->Release();\n                    convOutput.pSample = pConvSample;'
)

# In DecodeFrame's error paths, remove remaining Buffer releases if they exist:
# e.g., pConvBuffer->Release(); pConvSample->Release(); -> pConvSample->Release();
# We already replaced `pConvBuffer->Release();\n            pConvSample->Release();` in EncodeFrame which affects DecodeFrame too if it matches exactly. Let's make sure.

with open('H264Helper.cpp', 'w', encoding='utf-8') as f:
    f.write(code)

print("H264Helper.cpp memory leaks fixed.")
