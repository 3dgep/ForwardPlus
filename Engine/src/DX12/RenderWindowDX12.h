#pragma once

#include <RenderWindow.h>
#include "DescriptorHeapDX12.h"

class RenderDeviceDX12;

static const UINT NUM_BACK_BUFFERS = 2;

class RenderWindowDX12 : public RenderWindow
{
public:
    typedef RenderWindow base;

    RenderWindowDX12( Application& app, HWND hWindow, RenderDeviceDX12& device, const std::string& windowName, int windowWidth, int windowHeight, bool vSync );
    virtual ~RenderWindowDX12();

    virtual void ShowWindow();
    virtual void HideWindow();

    virtual void Present();

protected:
    virtual void CreateSwapChain();

    void ResizeSwapChainBuffers( int width, int height );

    virtual void OnPreRender( RenderEventArgs& e );
    virtual void OnRender( RenderEventArgs& e );
    virtual void OnPostRender( RenderEventArgs& e );

    virtual void OnMouseMoved( MouseMotionEventArgs& e );
    virtual void OnMouseLeave( EventArgs& e );

    virtual void OnResize( ResizeEventArgs& e );

    virtual void OnTerminate( EventArgs& e );

private:

    bool m_bIsMouseTracking; // Used to capture mouse enter/leave events.

    HWND m_hWindow;
    RenderDeviceDX12& m_RenderDevice;
    
    Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_pCommandList;

    Microsoft::WRL::ComPtr<IDXGISwapChain> m_pSwapChain;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pRenderTarget[NUM_BACK_BUFFERS];
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pDepthStencil;

    UINT m_CurrentBuffer;

    // Render Target View (RTV) descriptor heap.
    std::shared_ptr<DescriptorHeapDX12> m_pRTVDescriptorHeap;

    // Depth Stencil View (DSV) descriptor heap.
    std::shared_ptr<DescriptorHeapDX12> m_pDSVDescriptorHeap;

};