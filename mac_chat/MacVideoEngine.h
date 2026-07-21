#pragma once
#include <cstdint>
#include <cstddef>
#include <functional>
#include <vector>

class MacVideoEngine {
public:
    MacVideoEngine();
    ~MacVideoEngine();

    // Initialize Capture Session (Webcam)
    bool StartCapture();
    void StopCapture();

    // Screen Capture
    bool StartScreenCapture();
    void StopScreenCapture();

    // Window Capture
    bool StartWindowCapture(uint32_t windowId);
    void StopWindowCapture();

    // H.264 Encoder
    bool InitializeEncoder(int width, int height, int fps);
    void EncodeFrame(void* imageBuffer); // Takes CVImageBufferRef
    
    using VideoDataCallback = std::function<void(const uint8_t* data, size_t size, bool isH264)>;
    void SetVideoCallback(VideoDataCallback callback);
    void ProcessEncodedData(const uint8_t* data, size_t size, bool isH264);

    // H.264 Decoder
    bool InitializeDecoder();
    void DecodeFrame(const uint8_t* h264Data, size_t size);
    
    using DecodedFrameCallback = std::function<void(void* sampleBuffer)>;
    void SetDecodedCallback(DecodedFrameCallback callback);
    void ProcessDecodedFrame(void* sampleBuffer);

private:
    bool m_encoderInitialized;
    bool m_decoderInitialized;
    bool m_useJpeg;
    int m_encoderWidth;
    int m_encoderHeight;
    bool m_isCapturing;
    bool m_isScreenCapturing;
    bool m_isWindowCapturing;
    uint32_t m_targetWindowId;
    VideoDataCallback m_callback;
    DecodedFrameCallback m_decodedCallback;
    
    void* m_captureSession;       // AVCaptureSession*
    void* m_screenCaptureSession; // AVCaptureSession* for Screen
    void* m_videoOutputDelegate;  // id<AVCaptureVideoDataOutputSampleBufferDelegate>
    void* m_compressionSession;   // VTCompressionSessionRef
    void* m_decompressionSession; // VTDecompressionSessionRef
    void* m_windowCaptureTimer;   // dispatch_source_t
};
