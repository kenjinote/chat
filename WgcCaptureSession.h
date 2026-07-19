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

    bool StartForWindow(HWND hwnd, ID3D11Device* pDevice, ID3D11DeviceContext* pContext) {
        return Start(CreateItemForWindow(hwnd), pDevice, pContext);
    }

    bool StartForMonitor(HMONITOR hmonitor, ID3D11Device* pDevice, ID3D11DeviceContext* pContext) {
        return Start(CreateItemForMonitor(hmonitor), pDevice, pContext);
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
        m_d3dContext = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_d3dDevice = nullptr;
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

    ID3D11Device* GetDevice() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_d3dDevice.get();
    }

    bool GetLatestTexture(ID3D11Texture2D** ppTexture, UINT32& outWidth, UINT32& outHeight, bool* pIsNew = nullptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_latestTexture || !m_d3dDevice) return false;

        // COM pointer equality is unreliable for ID3D11Device across API boundaries.
        // Since we created the frame pool with m_d3dDevice, we can safely assume it matches.
        D3D11_TEXTURE2D_DESC desc;
        m_latestTexture->GetDesc(&desc);
        outWidth = desc.Width;
        outHeight = desc.Height;

        m_latestTexture.copy_to(ppTexture);
        if (pIsNew) {
            *pIsNew = m_isNewFrame;
            m_isNewFrame = false;
        }
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

    bool Start(winrt::Windows::Graphics::Capture::GraphicsCaptureItem item, ID3D11Device* pDevice, ID3D11DeviceContext* pContext) {
        Stop();
        if (!item || !pDevice || !pContext) return false;

        m_d3dDevice.copy_from(pDevice);
        m_d3dContext.copy_from(pContext);

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
            auto newSize = m_item.Size();
            if (m_lastSize.Width != newSize.Width || m_lastSize.Height != newSize.Height) {
                m_lastSize = newSize;
                m_framePool.Recreate(
                    m_device,
                    winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
                    2,
                    m_lastSize);
                return; // Wait for the next frame with the correct size
            }
            auto surface = frame.Surface();
            auto access = surface.as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
            winrt::com_ptr<ID3D11Texture2D> texture;
            if (SUCCEEDED(access->GetInterface(winrt::guid_of<ID3D11Texture2D>(), texture.put_void()))) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_latestTexture = texture;
                m_isNewFrame = true;
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
    winrt::com_ptr<ID3D11Texture2D> m_latestTexture = nullptr;
    bool m_isNewFrame = false;
    winrt::Windows::Graphics::SizeInt32 m_lastSize{ 0, 0 };
};
