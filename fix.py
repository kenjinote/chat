import re
with open('H264Helper.cpp', 'r', encoding='utf-8') as f:
    text = f.read()
text = text.replace('LogToFile', 'Log')
old_init = '''bool H264Encoder::Initialize(UINT32 width, UINT32 height, bool isScreenShare, ID3D11Device* pD3D11Device) {
    if (IsHardwareAccelerationEnabled()) {
        if (InitializeInternal(width, height, isScreenShare, true, pD3D11Device)) return true;
        Log(\
H264Encoder
HW
init
failed
falling
back
to
SW\);
    }
    return InitializeInternal(width, height, isScreenShare, false, nullptr);
}'''
new_init = '''bool H264Encoder::Initialize(UINT32 width, UINT32 height, bool isScreenShare, ID3D11Device* pD3D11Device) {
    bool canUseHW = IsHardwareAccelerationEnabled();
    if (isScreenShare && pD3D11Device == nullptr) {
        canUseHW = false;
    }
    if (canUseHW) {
        if (InitializeInternal(width, height, isScreenShare, true, pD3D11Device)) return true;
        Log(\
H264Encoder
HW
init
failed
falling
back
to
SW\);
    }
    return InitializeInternal(width, height, isScreenShare, false, nullptr);
}'''
text = text.replace(old_init, new_init)
old_d3d = '''    Log(\
H264Encoder::InitializeInternal
Setup
D3D
Manager
for
Encoder\);
    if (pDXGIManager && useHW) {
        hr = pEncoder->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)pDXGIManager);
        if (FAILED(hr)) Log(\
H264Encoder::InitializeInternal
MFT_MESSAGE_SET_D3D_MANAGER
FAILED
hr=\ + std::to_string(hr));
    }'''
new_d3d = '''    Log(\
H264Encoder::InitializeInternal
Setup
D3D
Manager
for
Encoder\);
    if (pDXGIManager && useHW) {
        hr = pEncoder->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)pDXGIManager);
        if (FAILED(hr)) {
            Log(\
H264Encoder::InitializeInternal
MFT_MESSAGE_SET_D3D_MANAGER
FAILED
hr=\ + std::to_string(hr));
            return false;
        }
    }'''
text = text.replace(old_d3d, new_d3d)
with open('H264Helper.cpp', 'w', encoding='utf-8') as f:
    f.write(text)
