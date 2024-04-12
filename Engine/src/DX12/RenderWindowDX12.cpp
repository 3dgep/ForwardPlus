#include <EnginePCH.h>

#include "../resource.h"

#include <Application.h>
#include <EngineTime.h>
#include <Camera.h>
#include <Texture.h>
#include <Rect.h>
#include <Material.h>

#include "d3dx12.h"
#include "RenderDeviceDX12.h"
#include "BufferDX12.h"

#include "RenderWindowDX12.h"

using Microsoft::WRL::ComPtr;

// Global Window Procedure function
static LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );

RenderWindowDX12::RenderWindowDX12( Application& app, HWND hWnd, RenderDeviceDX12& device, const std::string& windowName, int windowWidth, int windowHeight, bool vSync )
    : base( app, windowName, windowWidth, windowHeight, vSync )
    , m_hWindow( hWnd )
    , m_RenderDevice( device )
    , m_pDevice( device.GetDevice() )
    , m_pCommandList( device.GetCommandList() )
    , m_bIsMouseTracking( false )
    , m_CurrentBuffer(0)
{

    CreateSwapChain();

    // Initialize AntTweak bar.
//    TwInit(TW_DIRECT3D11, m_pDevice.Get());
}

RenderWindowDX12::~RenderWindowDX12()
{
    if ( m_pSwapChain )
    {
        // Apparently an exception is thrown when you release the swap chain if you don't do this.
        if ( FAILED( m_pSwapChain->SetFullscreenState( false, nullptr ) ) )
        {
            ReportError( "Failed to set swap chain to full-screen mode." );
        }
    }

    m_RenderDevice.WaitForPreviousFrame();
}

void RenderWindowDX12::ShowWindow()
{
    base::ShowWindow();

    ::ShowWindow( m_hWindow, SW_SHOWDEFAULT );
    ::BringWindowToTop( m_hWindow );

}

void RenderWindowDX12::HideWindow()
{
    base::HideWindow();

    ::ShowWindow( m_hWindow, SW_HIDE );
}

// This function was inspired by:
// http://www.rastertek.com/dx11tut03.html
static DXGI_RATIONAL QueryRefreshRate( UINT screenWidth, UINT screenHeight, BOOL vsync )
{
    DXGI_RATIONAL refreshRate = { 0, 1 };
    if ( vsync )
    {
        ComPtr<IDXGIFactory> factory;
        ComPtr<IDXGIAdapter> adapter;
        ComPtr<IDXGIOutput> adapterOutput;
        DXGI_MODE_DESC* displayModeList;

        // Create a DirectX graphics interface factory.
        if ( FAILED( CreateDXGIFactory( __uuidof( IDXGIFactory ), &factory ) ) )
        {
            ReportError( "Failed to create DXGIFactory." );
        }

        if ( FAILED( factory->EnumAdapters( 0, &adapter ) ) )
        {
            ReportError( "Failed to enumerate adapters." );
        }

        if ( FAILED( adapter->EnumOutputs( 0, &adapterOutput ) ) )
        {
            ReportError( "Failed to enumerate adapter outputs." );
        }

        UINT numDisplayModes;
        if ( FAILED( adapterOutput->GetDisplayModeList( DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, NULL ) ) )
        {
            ReportError( "Failed to query display modes." );
        }

        displayModeList = new DXGI_MODE_DESC[numDisplayModes];
        assert( displayModeList );

        if ( FAILED( adapterOutput->GetDisplayModeList( DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, displayModeList ) ) )
        {
            ReportError( "Failed to query display modes." );
        }

        // Now store the refresh rate of the monitor that matches the width and height of the requested screen.
        for ( UINT i = 0; i < numDisplayModes; ++i )
        {
            if ( displayModeList[i].Width == screenWidth && displayModeList[i].Height == screenHeight )
            {
                refreshRate = displayModeList[i].RefreshRate;
            }
        }

        delete[] displayModeList;
    }

    return refreshRate;
}

void RenderWindowDX12::CreateSwapChain()
{
    UINT windowWidth = GetWindowWidth();
    UINT windowHeight = GetWindowHeight();
    bool vSync = IsVSync();


    //std::string windowName = GetWindowName();

    DXGI_RATIONAL refreshRate = QueryRefreshRate( windowWidth, windowHeight, vSync );

    // Create a swap chain with a single render target.
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = NUM_BACK_BUFFERS;
    swapChainDesc.BufferDesc.Width = windowWidth;
    swapChainDesc.BufferDesc.Height = windowHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate = refreshRate;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = m_hWindow;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    // Allow switching between window and full-screen mode by pressing Alt+Enter
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    m_pSwapChain = m_RenderDevice.CreateSwapChain( swapChainDesc );
    
    // Create a descriptor heap for the render target view (RTV)
    m_pRTVDescriptorHeap = std::make_shared<DescriptorHeapDX12>( m_pDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NUM_BACK_BUFFERS );
    CPUDescriptorHandleDX12& cpuHandle = m_pRTVDescriptorHeap->GetCPUHandle();

    for ( UINT i = 0; i < NUM_BACK_BUFFERS; ++i )
    {
        if ( FAILED( m_pSwapChain->GetBuffer( i, __uuidof( ID3D12Resource ), &m_pRenderTarget[i] ) ) )
        {
            ReportError( "Failed to get back buffer from swap chain." );
        }
        m_pDevice->CreateRenderTargetView( m_pRenderTarget[i].Get(), nullptr, cpuHandle++ );
    }

    // Create a descriptor heap for the Depth Stencil View
    m_pDSVDescriptorHeap = std::make_shared<DescriptorHeapDX12>( m_pDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV );

    // Create the depth-stencil resource for the scene.
    D3D12_RESOURCE_DESC depthStencilBufferDesc = {};
    depthStencilBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilBufferDesc.Alignment = 0;
    depthStencilBufferDesc.Width = windowWidth;
    depthStencilBufferDesc.Height = windowHeight;
    depthStencilBufferDesc.DepthOrArraySize = 1;
    depthStencilBufferDesc.MipLevels = 1;
    depthStencilBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilBufferDesc.SampleDesc.Count = 1;
    depthStencilBufferDesc.SampleDesc.Quality = 0;
    depthStencilBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    // Create a resource for the depth-stencil buffer.
    if ( FAILED( m_pDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT ),
                D3D12_HEAP_FLAG_NONE,
                &depthStencilBufferDesc,
                D3D12_RESOURCE_STATE_COMMON,
                &clearValue,
                __uuidof( ID3D12Resource ),
                &m_pDepthStencil ) ) )
    {
        ReportError( "Failed to create Depth-Stencil resource" );
    }

    m_pDevice->CreateDepthStencilView( m_pDepthStencil.Get(), nullptr, m_pDSVDescriptorHeap->GetCPUHandle() );
}


void RenderWindowDX12::ResizeSwapChainBuffers( int width, int height )
{
    assert( m_pSwapChain && m_pCommandList );

    // Don't allow 0 size buffers.
    width = std::max( 1, width );
    height = std::max( 1, height );

    // Disable any render targets we currently have bound.
    // TODO: This should only be done if the swap chain's back buffer is bound as the
    // current render target?
    m_pCommandList->OMSetRenderTargets( 0, nullptr, false, nullptr );

    m_RenderDevice.CommitCommandList();
    // Wait for the resources to become unused before releasing the render targets.
    m_RenderDevice.WaitForPreviousFrame();

    // Release any render targets we have.
    for ( int i = 0; i < NUM_BACK_BUFFERS; ++i )
    {
        m_pRenderTarget[i].Reset();
    }

    m_pDepthStencil.Reset();

    if ( FAILED( m_pSwapChain->ResizeBuffers( 0, width, height, DXGI_FORMAT_UNKNOWN, 0 ) ) )
    {
        ReportError( "Failed to resize the swap chain." );
    }

    CPUDescriptorHandleDX12 cpuHandle = m_pRTVDescriptorHeap->GetCPUHandleStart();
    for ( UINT i = 0; i < NUM_BACK_BUFFERS; ++i )
    {
        if ( FAILED( m_pSwapChain->GetBuffer( i, __uuidof( ID3D12Resource ), &m_pRenderTarget[i] ) ) )
        {
            ReportError( "Failed to get back buffer from swap chain." );
        }
        m_pDevice->CreateRenderTargetView( m_pRenderTarget[i].Get(), nullptr, cpuHandle++ );
    }

    // Create the depth-stencil resource for the scene.
    D3D12_RESOURCE_DESC depthStencilBufferDesc = {};
    depthStencilBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilBufferDesc.Alignment = 0;
    depthStencilBufferDesc.Width = width;
    depthStencilBufferDesc.Height = height;
    depthStencilBufferDesc.DepthOrArraySize = 1;
    depthStencilBufferDesc.MipLevels = 1;
    depthStencilBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilBufferDesc.SampleDesc.Count = 1;
    depthStencilBufferDesc.SampleDesc.Quality = 0;
    depthStencilBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    // Create a resource for the depth-stencil buffer.
    if ( FAILED( m_pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT ),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilBufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &clearValue,
        __uuidof( ID3D12Resource ),
        &m_pDepthStencil ) ) )
    {
        ReportError( "Failed to create Depth-Stencil resource" );
    }

    m_pDevice->CreateDepthStencilView( m_pDepthStencil.Get(), nullptr, m_pDSVDescriptorHeap->GetCPUHandle() );

    m_CurrentBuffer = 0;
}

void RenderWindowDX12::Present()
{
    m_pCommandList->ResourceBarrier( 1, &CD3DX12_RESOURCE_BARRIER::Transition( m_pRenderTarget[m_CurrentBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT ) );

    m_RenderDevice.CommitCommandList();

    if ( IsVSync() )
    {
        if ( FAILED( m_pSwapChain->Present( 1, 0 ) ) )
        {
            ReportError( "Failed to present the swap chain." );
        }
    }
    else
    {
        if ( FAILED( m_pSwapChain->Present( 0, 0 ) ) )
        {
            ReportError( "Failed to present the swap chain." );
        }
    }

    m_CurrentBuffer = ( m_CurrentBuffer + 1 ) % NUM_BACK_BUFFERS;
}

void RenderWindowDX12::OnPreRender( RenderEventArgs& e )
{
    // Transition the back buffer to be used as a render target.
    m_pCommandList->ResourceBarrier( 1, &CD3DX12_RESOURCE_BARRIER::Transition( m_pRenderTarget[m_CurrentBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET ) );

    m_RenderDevice.CommitCommandListAndResetAllocator();

    base::OnPreRender( e );
}

void RenderWindowDX12::OnRender( RenderEventArgs& e )
{
    // Bind the render target view and the depth stencil view before rendering
//    m_pDeviceContext->OMSetRenderTargets( 1, &m_pRenderTargetView, m_pDepthStencilView );

    base::OnRender( e );
}

void RenderWindowDX12::OnPostRender( RenderEventArgs& e )
{
    // Draw the AntTweakBar
//    TwDraw();

    m_RenderDevice.CommitCommandList();

    base::OnPostRender( e );
}

void RenderWindowDX12::OnMouseMoved( MouseMotionEventArgs& e )
{
    if ( !m_bIsMouseTracking )
    {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof( TRACKMOUSEEVENT );
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWindow;
        if ( TrackMouseEvent( &tme ) )
        {
            m_bIsMouseTracking = true;
        }
    }

    base::OnMouseMoved( e );
}

void RenderWindowDX12::OnMouseLeave( EventArgs& e )
{
    m_bIsMouseTracking = false;
    base::OnMouseLeave( e );
}

void RenderWindowDX12::OnResize( ResizeEventArgs& e )
{
    ResizeSwapChainBuffers( e.Width, e.Height );
    base::OnResize( e );
}


void RenderWindowDX12::OnTerminate( EventArgs& e )
{
//    TwTerminate();

    base::OnTerminate( e );
}
