#include "H264Helper.h"

int main() {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    MFStartup(MF_VERSION);

    H264Encoder enc;
    if (!enc.Initialize(640, 480)) {
        std::cout << "Enc Init Failed" << std::endl;
        return 1;
    }
    std::cout << "Enc Init Success" << std::endl;

    H264Decoder dec;
    if (!dec.Initialize(640, 480)) {
        std::cout << "Dec Init Failed" << std::endl;
        return 1;
    }
    std::cout << "Dec Init Success" << std::endl;

    std::vector<uint8_t> dummyFrame(640 * 480 * 4, 128); // Grey RGB32
    std::vector<uint8_t> h264;
    
    // We may need to push a few frames before the encoder outputs NALUs (because of B-frames or buffering)
    // H.264 Encoder MFT requires a few frames before Output
    for (int i=0; i<60; i++) {
        enc.EncodeFrame(dummyFrame.data(), dummyFrame.size(), h264);
        if (!h264.empty()) {
            std::cout << "Encoded H264 size: " << h264.size() << "\n";
            std::vector<uint8_t> rgbOut;
            dec.DecodeFrame(h264, rgbOut);
            if (!rgbOut.empty()) {
                std::cout << "Decoded RGB32 size: " << rgbOut.size() << "\n";
            }
        }
    }

    MFShutdown();
    CoUninitialize();
    std::cout << "Done!" << std::endl;
    return 0;
}
