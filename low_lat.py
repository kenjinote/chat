import re

with open('H264Helper.cpp', 'r') as f:
    content = f.read()

# Insert Low Latency attribute after CoCreateInstance for encoder
replacement = """    HRESULT hr = CoCreateInstance(CLSID_CMSH264EncoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pEncoder));
    if (FAILED(hr)) return false;

    IMFAttributes* pAttributes = NULL;
    if (SUCCEEDED(pEncoder->GetAttributes(&pAttributes))) {
        pAttributes->SetUINT32(MF_LOW_LATENCY, 1);
        pAttributes->Release();
    }
"""

content = content.replace(
    '    HRESULT hr = CoCreateInstance(CLSID_CMSH264EncoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pEncoder));\n    if (FAILED(hr)) return false;\n',
    replacement
)

with open('H264Helper.cpp', 'w') as f:
    f.write(content)

with open('test_helper.cpp', 'r') as f:
    content2 = f.read()

content2 = content2.replace('i<10', 'i<60')
content2 = content2.replace('return 0;', 'std::cout << "Done!" << std::endl;\n    return 0;')

with open('test_helper.cpp', 'w') as f:
    f.write(content2)
