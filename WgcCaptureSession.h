#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
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
        m_item = nullptr;
        m_device = nullptr;
        m_d3dDevice = nullptr;
        m_d3dContext = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_latestTexture = nullptr;
        }
    }

    bool IsActive() const {
        return m_session != nullptr;
    }

    bool GetLatestFrame(std::vector<uint8_t>& rgb32Data, UINT32& outWidth, UINT32& outHeight) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_latestTexture) return false;

        D3D11_TEXTURE2D_DESC desc;
        m_latestTexture->GetDesc(&desc);

        // Map texture to CPU
        winrt::com_ptr<ID3D11Texture2D> stagingTex;
        D3D11_TEXTURE2D_DESC stagingDesc = desc;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.BindFlags = 0;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        stagingDesc.MiscFlags = 0;

        if (FAILED(m_d3dDevice->CreateTexture2D(&stagingDesc, nullptr, stagingTex.put()))) {
            return false;
        }

        m_d3dContext->CopyResource(stagingTex.get(), m_latestTexture.get());

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_d3dContext->Map(stagingTex.get(), 0, D3D11_MAP_READ, 0, &mapped))) {
            outWidth = desc.Width;
            outHeight = desc.Height;
            rgb32Data.resize(outWidth * outHeight * 4);

            const uint8_t* src = (const uint8_t*)mapped.pData;
            uint8_t* dst = rgb32Data.data();
            for (UINT32 y = 0; y < outHeight; ++y) {
                memcpy(dst + y * outWidth * 4, src + y * mapped.RowPitch, outWidth * 4);
            }

            m_d3dContext->Unmap(stagingTex.get(), 0);
            return true;
        }
        return false;
    }

private:
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem CreateItemForWindow(HWND hwnd) {
        auto interop_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
        winrt::check_hresult(interop_factory->CreateForWindow(hwnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(item)));
        return item;
    }

    winrt::Windows::Graphics::Capture::GraphicsCaptureItem CreateItemForMonitor(HMONITOR hmonitor) {
        auto interop_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
        winrt::check_hresult(interop_factory->CreateForMonitor(hmonitor, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(item)));
        return item;
    }

    bool Start(winrt::Windows::Graphics::Capture::GraphicsCaptureItem item) {
        Stop();

        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
        
        if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, m_d3dDevice.put(), nullptr, m_d3dContext.put()))) {
            return false;
        }

        winrt::com_ptr<IDXGIDevice> dxgiDevice = m_d3dDevice.as<IDXGIDevice>();
        winrt::com_ptr<::IInspectable> inspectable;
        if (FAILED(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), inspectable.put()))) {
            return false;
        }
        m_device = inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();

        m_item = item;
        m_item.Closed({ this, &WgcCaptureSession::OnClosed });

        m_framePool = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::CreateFreeThreaded(
            m_device,
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            m_item.Size());

        m_framePool.FrameArrived({ this, &WgcCaptureSession::OnFrameArrived });
        m_session = m_framePool.CreateCaptureSession(m_item);
        m_session.StartCapture();

        return true;
    }

    void OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const& args) {
        auto frame = sender.TryGetNextFrame();
        if (frame) {
            auto surface = frame.Surface();
            auto access = surface.as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
            winrt::com_ptr<ID3D11Texture2D> texture;
            if (SUCCEEDED(access->GetInterface(winrt::guid_of<ID3D11Texture2D>(), texture.put_void()))) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_latestTexture = texture;
            }
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
    winrt::com_ptr<ID3D11Texture2D> m_latestTexture{ nullptr };
};
