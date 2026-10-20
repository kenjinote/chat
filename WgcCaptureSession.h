#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#include <vector>
#include <mutex>

extern "C" {
    HRESULT WINAPI CreateDirect3D11DeviceFromDXGIDevice(IDXGIDevice* dxgiDevice, IInspectable** graphicsDevice);
}

class WgcCaptureSession {
public:
    WgcCaptureSession() = default;
    ~WgcCaptureSession() {
        Stop();
    }

    bool StartForWindow(HWND hwnd) {
        return Start(CreateItemForWindow(hwnd));
    }

    bool StartForMonitor(HMONITOR hmonitor) {
        return Start(CreateItemForMonitor(hmonitor));
    }

    void Stop() {
        if (m_session) {
            m_session.Close();
            m_session = nullptr;
        }
        if (m_framePool) {
            m_framePool.Close();
            m_framePool = nullptr;
        }
        
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_item = nullptr;
            m_device = nullptr;
            m_d3dContext = nullptr;
            m_d3dDevice = nullptr;
            m_gpuTexture = nullptr;
            m_encodeTexture = nullptr;
            m_stagingTexture = nullptr;
            m_latestPixels.clear();
        }
    }

    bool IsActive() const {
        return m_session != nullptr;
    }

    ID3D11Device* GetDevice() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_d3dDevice.get();
    }

    bool CheckNewFrame(UINT32& outWidth, UINT32& outHeight, bool* pIsNew = nullptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_gpuTexture && m_latestPixels.empty()) return false;

        outWidth = m_latestWidth;
        outHeight = m_latestHeight;

        if (pIsNew) {
            *pIsNew = m_isNewFrame;
            m_isNewFrame = false;
        }
        return true;
    }

    bool GetLatestTexture(winrt::com_ptr<ID3D11Texture2D>& outTexture, UINT32& outWidth, UINT32& outHeight) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_gpuTexture || !m_d3dDevice || !m_d3dContext) return false;

        D3D11_TEXTURE2D_DESC desc;
        m_gpuTexture->GetDesc(&desc);

        if (m_encodeTexture) {
            D3D11_TEXTURE2D_DESC encDesc;
            m_encodeTexture->GetDesc(&encDesc);
            if (encDesc.Width != desc.Width || encDesc.Height != desc.Height || encDesc.Format != desc.Format) {
                m_encodeTexture = nullptr;
            }
        }

        if (!m_encodeTexture) {
            D3D11_TEXTURE2D_DESC encDesc = {};
            encDesc.Width = desc.Width;
            encDesc.Height = desc.Height;
            encDesc.MipLevels = 1;
            encDesc.ArraySize = 1;
            encDesc.Format = desc.Format;
            encDesc.SampleDesc.Count = 1;
            encDesc.Usage = D3D11_USAGE_DEFAULT;
            encDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            encDesc.CPUAccessFlags = 0;
            encDesc.MiscFlags = 0;
            if (FAILED(m_d3dDevice->CreateTexture2D(&encDesc, nullptr, m_encodeTexture.put()))) {
                return false;
            }
        }

        m_d3dContext->CopyResource(m_encodeTexture.get(), m_gpuTexture.get());
        m_d3dContext->Flush();

        outTexture = m_encodeTexture;
        outWidth = m_latestWidth;
        outHeight = m_latestHeight;
        return true;
    }

    bool GetLatestFrame(std::vector<uint8_t>& rgb32Data, UINT32& outWidth, UINT32& outHeight) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_gpuTexture && m_d3dDevice && m_d3dContext) {
            D3D11_TEXTURE2D_DESC desc;
            m_gpuTexture->GetDesc(&desc);

            if (m_stagingTexture) {
                D3D11_TEXTURE2D_DESC stagingDesc;
                m_stagingTexture->GetDesc(&stagingDesc);
                if (stagingDesc.Width != desc.Width || stagingDesc.Height != desc.Height) {
                    m_stagingTexture = nullptr;
                }
            }

            if (!m_stagingTexture) {
                D3D11_TEXTURE2D_DESC stagingDesc = desc;
                stagingDesc.Usage = D3D11_USAGE_STAGING;
                stagingDesc.BindFlags = 0;
                stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                stagingDesc.MiscFlags = 0;
                if (FAILED(m_d3dDevice->CreateTexture2D(&stagingDesc, nullptr, m_stagingTexture.put()))) {
                    m_stagingTexture = nullptr;
                }
            }

            if (m_stagingTexture) {
                m_d3dContext->CopyResource(m_stagingTexture.get(), m_gpuTexture.get());
                m_d3dContext->Flush();

                D3D11_MAPPED_SUBRESOURCE mapped;
                if (SUCCEEDED(m_d3dContext->Map(m_stagingTexture.get(), 0, D3D11_MAP_READ, 0, &mapped))) {
                    outWidth = desc.Width;
                    outHeight = desc.Height;
                    rgb32Data.resize(outWidth * outHeight * 4);

                    const uint8_t* src = (const uint8_t*)mapped.pData;
                    uint8_t* dst = rgb32Data.data();
                    for (UINT32 y = 0; y < outHeight; ++y) {
                        memcpy(dst + y * outWidth * 4, src + y * mapped.RowPitch, outWidth * 4);
                    }

                    m_d3dContext->Unmap(m_stagingTexture.get(), 0);
                    return true;
                }
            }
        }

        if (m_latestPixels.empty()) return false;
        outWidth = m_latestWidth;
        outHeight = m_latestHeight;
        rgb32Data = m_latestPixels;
        return true;
    }

public:
    static bool IsWindowCapturable(HWND hwnd) {
        if (!hwnd) return false;
        if (!IsWindow(hwnd)) return false;
        if (!IsWindowVisible(hwnd)) return false;
        if (IsIconic(hwnd)) return false;

        // Check for cloaked windows (UWP/Virtual Desktop/etc.)
        BOOL cloaked = FALSE;
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked) return false;

        return true;
    }

    winrt::Windows::Graphics::Capture::GraphicsCaptureItem CreateItemForWindow(HWND hwnd) {
        if (!IsWindowCapturable(hwnd)) return { nullptr };
        try {
            auto interop_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
            winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
            winrt::check_hresult(interop_factory->CreateForWindow(hwnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(item)));
            return item;
        } catch (winrt::hresult_error const&) {
            return { nullptr };
        }
    }

    winrt::Windows::Graphics::Capture::GraphicsCaptureItem CreateItemForMonitor(HMONITOR hmonitor) {
        try {
            auto interop_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
            winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
            winrt::check_hresult(interop_factory->CreateForMonitor(hmonitor, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(item)));
            return item;
        } catch (winrt::hresult_error const&) {
            return { nullptr };
        }
    }

    bool Start(winrt::Windows::Graphics::Capture::GraphicsCaptureItem item) {
        Stop();
        if (!item) return false;

        UINT creationFlags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
        D3D_FEATURE_LEVEL featureLevel;
        winrt::com_ptr<ID3D11Device> d3dDevice;
        winrt::com_ptr<ID3D11DeviceContext> d3dContext;
        
        if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, d3dDevice.put(), &featureLevel, d3dContext.put()))) {
            creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
            if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, d3dDevice.put(), &featureLevel, d3dContext.put()))) {
                return false;
            }
        }

        winrt::com_ptr<ID3D10Multithread> multithread;
        if (SUCCEEDED(d3dDevice->QueryInterface(__uuidof(ID3D10Multithread), multithread.put_void()))) {
            multithread->SetMultithreadProtected(TRUE);
        }
        
        m_d3dDevice = d3dDevice;
        m_d3dContext = d3dContext;

        winrt::com_ptr<IDXGIDevice> dxgiDevice = m_d3dDevice.as<IDXGIDevice>();
        winrt::com_ptr<::IInspectable> inspectable;
        if (FAILED(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), inspectable.put()))) {
            return false;
        }
        m_device = inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();

        m_item = item;
        m_item.Closed({ this, &WgcCaptureSession::OnClosed });

        m_lastSize = m_item.Size();
        m_framePool = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::CreateFreeThreaded(
            m_device,
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            m_lastSize);

        m_framePool.FrameArrived({ this, &WgcCaptureSession::OnFrameArrived });
        m_session = m_framePool.CreateCaptureSession(m_item);
        m_session.StartCapture();

        return true;
    }

    void OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const& args) {
        auto frame = sender.TryGetNextFrame();
        if (frame) {
            if (!m_item) {
                frame.Close();
                return;
            }
            auto newSize = m_item.Size();
            if (m_lastSize.Width != newSize.Width || m_lastSize.Height != newSize.Height) {
                m_lastSize = newSize;
                m_framePool.Recreate(
                    m_device,
                    winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
                    2,
                    m_lastSize);
                try { frame.Close(); } catch (...) {}
                return; // Wait for the next frame with the correct size
            }
            auto d3dDevice = m_d3dDevice;
            auto d3dContext = m_d3dContext;
            if (!d3dDevice || !d3dContext) {
                try { frame.Close(); } catch (...) {}
                return;
            }

            auto surface = frame.Surface();
            auto access = surface.as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
            winrt::com_ptr<ID3D11Texture2D> texture;
            if (SUCCEEDED(access->GetInterface(winrt::guid_of<ID3D11Texture2D>(), texture.put_void()))) {
                D3D11_TEXTURE2D_DESC desc;
                texture->GetDesc(&desc);

                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_gpuTexture) {
                    D3D11_TEXTURE2D_DESC gpuDesc;
                    m_gpuTexture->GetDesc(&gpuDesc);
                    if (gpuDesc.Width != desc.Width || gpuDesc.Height != desc.Height || gpuDesc.Format != desc.Format) {
                        m_gpuTexture = nullptr;
                    }
                }

                if (!m_gpuTexture) {
                    D3D11_TEXTURE2D_DESC gpuDesc = {};
                    gpuDesc.Width = desc.Width;
                    gpuDesc.Height = desc.Height;
                    gpuDesc.MipLevels = 1;
                    gpuDesc.ArraySize = 1;
                    gpuDesc.Format = desc.Format;
                    gpuDesc.SampleDesc.Count = 1;
                    gpuDesc.Usage = D3D11_USAGE_DEFAULT;
                    gpuDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
                    gpuDesc.CPUAccessFlags = 0;
                    gpuDesc.MiscFlags = 0;
                    if (FAILED(d3dDevice->CreateTexture2D(&gpuDesc, nullptr, m_gpuTexture.put()))) {
                        m_gpuTexture = nullptr;
                    }
                }

                if (m_gpuTexture) {
                    d3dContext->CopyResource(m_gpuTexture.get(), texture.get());
                    d3dContext->Flush();
                    m_latestWidth = desc.Width;
                    m_latestHeight = desc.Height;
                    m_isNewFrame = true;
                }
            }
            try { frame.Close(); } catch (...) {}
        }
    }

    void OnClosed(winrt::Windows::Graphics::Capture::GraphicsCaptureItem const& sender, winrt::Windows::Foundation::IInspectable const& args) {
        Stop();
    }

    std::mutex m_mutex;
    winrt::com_ptr<ID3D11Device> m_d3dDevice;
    winrt::com_ptr<ID3D11DeviceContext> m_d3dContext;
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_item{ nullptr };
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool{ nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_session{ nullptr };
    winrt::Windows::Graphics::SizeInt32 m_lastSize{ 0, 0 };
    winrt::com_ptr<ID3D11Texture2D> m_gpuTexture = nullptr;
    winrt::com_ptr<ID3D11Texture2D> m_encodeTexture = nullptr;
    winrt::com_ptr<ID3D11Texture2D> m_stagingTexture = nullptr;
    std::vector<uint8_t> m_latestPixels;
    UINT32 m_latestWidth = 0;
    UINT32 m_latestHeight = 0;
    bool m_isNewFrame = false;
};
