#include <EnginePCH.h>


#include "../resource.h"

#include <Application.h>
#include <ConstantBuffer.h> // These should be replaced with the DX12 versions when they become available 
#include <StructuredBuffer.h>

#include "d3dx12.h"
#include "RenderWindowDX12.h"
#include "BufferDX12.h"

#include "RenderDeviceDX12.h"

using Microsoft::WRL::ComPtr;

RenderDeviceDX12::RenderDeviceDX12( Application& app )
    : m_App( app )
{
    CreateDevice( app.GetModuleHandle() );
}

RenderDeviceDX12::~RenderDeviceDX12()
{
}

void RenderDeviceDX12::CreateDevice( HINSTANCE hInstance )
{
#ifdef _DEBUG
    // Enable the D3D12 debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        if ( FAILED( D3D12GetDebugInterface( __uuidof( ID3D12Debug ), &debugController ) ) )
        {
            ReportError( "Failed to retrieve the D3D12 debug interface." );
        }
        debugController->EnableDebugLayer();
    }
#endif

    if ( FAILED( CreateDXGIFactory1( __uuidof( IDXGIFactory4 ), &m_pFactory ) ) )
    {
        ReportError( "Failed to create DXGI Factory." );
    }

    // First try to create a hardware device.
    if ( FAILED( D3D12CreateDevice( nullptr, D3D_FEATURE_LEVEL_11_0, __uuidof( ID3D12Device ), &m_pDevice ) ) )
    {
        // If no hardware device exists, try to create a WARP device.
        ComPtr<IDXGIAdapter> warpAdapter;
        if ( FAILED( m_pFactory->EnumWarpAdapter( __uuidof( IDXGIAdapter ), &warpAdapter ) ) )
        {
            ReportError( "Failed to enumerate warp adapter." );
        }

        // If that failed, report an error and throw an exception. Maybe the application can fall-back to DX11.
        if ( FAILED( D3D12CreateDevice( warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof( ID3D12Device ), &m_pDevice ) ) )
        {
            ReportError( "Failed to create a DirectX 12 device." );
        }
    }

    // Create the command queue that will be used to execute the command lists.
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    if ( FAILED( m_pDevice->CreateCommandQueue( &commandQueueDesc, __uuidof( ID3D12CommandQueue ), &m_pCommandQueue ) ) )
    {
        ReportError( "Failed to create command queue." );
    }

    // Create GPU fence for synchronizing the render loop.
    if ( FAILED( m_pDevice->CreateFence( 0, D3D12_FENCE_FLAG_NONE, __uuidof( ID3D12Fence ), &m_pFence ) ) )
    {
        ReportError( "Could not create GPU fence." );
    }

    m_uiCurrentFence = 1;

    // Load a default vertex shader and pixel shader (required for creating the pipeline state object).
    std::string defaultShaderSource = GetStringResource( DEFAULT_SHADER, "Shader" );

    if ( defaultShaderSource.empty() )
    {
        ReportError( "Could not load default shader from resources." );
    }

#if defined(_DEBUG)
    UINT shaderCompilerFlags = D3DCOMPILE_DEBUG;
#else
    UINT shaderCompilerFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    ComPtr<ID3DBlob> pErrorBlob;
    if ( FAILED( D3DCompile( defaultShaderSource.c_str(), defaultShaderSource.size(), "DefaultShader.hlsl", nullptr, nullptr, "VS_main", "vs_5_0", shaderCompilerFlags, 0, &m_pVertexShaderBlob, &pErrorBlob ) ) )
    {
        std::string errorString( (const char*)pErrorBlob->GetBufferPointer(), pErrorBlob->GetBufferSize() );
        ReportError( errorString );
    }

    m_VertexShaderByteCode.pShaderBytecode = m_pVertexShaderBlob->GetBufferPointer();
    m_VertexShaderByteCode.BytecodeLength = m_pVertexShaderBlob->GetBufferSize();

    if ( FAILED( D3DCompile( defaultShaderSource.c_str(), defaultShaderSource.size(), "DefaultShader.hlsl", nullptr, nullptr, "PS_main", "ps_5_0", shaderCompilerFlags, 0, &m_pPixelShaderBlob, &pErrorBlob ) ) )
    {
        std::string errorString( (const char*)pErrorBlob->GetBufferPointer(), pErrorBlob->GetBufferSize() );
        ReportError( errorString );
    }

    m_PixelShaderByteCode.pShaderBytecode = m_pPixelShaderBlob->GetBufferPointer();
    m_PixelShaderByteCode.BytecodeLength = m_pPixelShaderBlob->GetBufferSize();


    // Create an input layout that matches the input expected from the default shader.
    const D3D12_INPUT_ELEMENT_DESC defaultShaderInputElements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_INPUT_LAYOUT_DESC defaultShaderInputLayout;
    defaultShaderInputLayout.pInputElementDescs = defaultShaderInputElements;
    defaultShaderInputLayout.NumElements = _countof( defaultShaderInputElements );

    // Create an empty root signature.
    ComPtr<ID3DBlob> pRootSignatureBlob;
    CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
    descRootSignature.Init( 0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT );
    if ( FAILED( D3D12SerializeRootSignature( &descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pRootSignatureBlob, &pErrorBlob ) ) )
    {
        ReportError( "Could not serialize empty root signature." );
    }

    if ( FAILED( m_pDevice->CreateRootSignature( 0, pRootSignatureBlob->GetBufferPointer(), pRootSignatureBlob->GetBufferSize(), __uuidof( ID3D12RootSignature ), &m_pRootSignature ) ) )
    {
        ReportError( "Could not create root signature." );
    }

    // Create a default pipeline state object.
    CD3DX12_BLEND_DESC blendDesc( D3D12_DEFAULT );

    CD3DX12_RASTERIZER_DESC rasterizerDesc( D3D12_DEFAULT );

    CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc( D3D12_DEFAULT );
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
    pipelineStateDesc.pRootSignature = m_pRootSignature.Get();
    pipelineStateDesc.VS = m_VertexShaderByteCode;
    pipelineStateDesc.PS = m_PixelShaderByteCode;
    pipelineStateDesc.InputLayout = defaultShaderInputLayout;
    pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateDesc.BlendState = blendDesc;
    pipelineStateDesc.RasterizerState = rasterizerDesc;
    pipelineStateDesc.DepthStencilState = depthStencilDesc;
    pipelineStateDesc.NumRenderTargets = 1;
    pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // Should match the format that was used when creating the swap chain.
    pipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;		  // Match the format of the Depth Stencil View that will be created later.
    pipelineStateDesc.SampleDesc.Count = 1;
    pipelineStateDesc.SampleDesc.Quality = 0;
    pipelineStateDesc.SampleMask = UINT_MAX;

    if ( FAILED( m_pDevice->CreateGraphicsPipelineState( &pipelineStateDesc, __uuidof( ID3D12PipelineState ), &m_pPipelineState ) ) )
    {
        ReportError( "Failed to create graphics pipeline state." );
    }

    // Create a command allocator. This is required to create command lists.
    if ( FAILED( m_pDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof( ID3D12CommandAllocator ), &m_pCommandAllocator ) ) )
    {
        ReportError( "Failed to create command allocator." );
    }

    // Create a default command list (only used by the render window).
    if ( FAILED( m_pDevice->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(), m_pPipelineState.Get(), __uuidof( ID3D12GraphicsCommandList ), &m_pCommandList ) ) )
    {
        ReportError( "Failed to create graphics command list." );
    }
}

Microsoft::WRL::ComPtr<ID3D12Device> RenderDeviceDX12::GetDevice() const
{
    return m_pDevice;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> RenderDeviceDX12::GetCommandList() const
{
    return m_pCommandList;
}

Microsoft::WRL::ComPtr<IDXGISwapChain> RenderDeviceDX12::CreateSwapChain( const DXGI_SWAP_CHAIN_DESC& swapChainDesc )
{
    DXGI_SWAP_CHAIN_DESC* pSwapChainDesc = const_cast<DXGI_SWAP_CHAIN_DESC*>( &swapChainDesc );

    // Create the swap chain with the command queue so that the swap chain will force
    // the command queue to be flushed before a buffer swap.
    ComPtr<IDXGISwapChain> pSwapChain;

    if ( FAILED( m_pFactory->CreateSwapChain( m_pCommandQueue.Get(), pSwapChainDesc, &pSwapChain ) ) )
    {
        ReportError( "Failed to create swap chain." );
    }

    return pSwapChain;
}

void RenderDeviceDX12::CommitCommandList()
{
    if ( FAILED( m_pCommandList->Close() ) )
    {
        ReportError( "Failed to close command list." );
    }

    ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
    m_pCommandQueue->ExecuteCommandLists( 1, ppCommandLists );

    if ( FAILED( m_pCommandList->Reset( m_pCommandAllocator.Get(), m_pPipelineState.Get() ) ) )
    {
        ReportError( "Failed to reset the command list." );
    }
}

void RenderDeviceDX12::CommitCommandListAndResetAllocator()
{
    if ( FAILED( m_pCommandList->Close() ) )
    {
        ReportError( "Failed to close command list." );
    }

    ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
    m_pCommandQueue->ExecuteCommandLists( _countof( ppCommandLists ), ppCommandLists );

    // Wait for the previous commands to complete execution before resetting
    // the command allocator.
    WaitForPreviousFrame();

    if ( FAILED( m_pCommandAllocator->Reset() ) )
    {
        ReportError( "Failed to reset the command allocator." );
    }

    if ( FAILED( m_pCommandList->Reset( m_pCommandAllocator.Get(), m_pPipelineState.Get() ) ) )
    {
        ReportError( "Failed to reset the command list." );
    }
}

void RenderDeviceDX12::WaitForPreviousFrame()
{
    assert( m_pFence && m_pCommandQueue );

    const UINT64 thisFence = m_uiCurrentFence;
    const UINT64 lastCompletedFence = m_pFence->GetCompletedValue();

    m_pCommandQueue->Signal( m_pFence.Get(), m_uiCurrentFence );
    m_uiCurrentFence++;

    if ( lastCompletedFence < thisFence )
    {
        HANDLE eventHandle = CreateEventEx( NULL, NULL, FALSE, EVENT_ALL_ACCESS );
        assert( eventHandle != NULL );
        if ( FAILED( m_pFence->SetEventOnCompletion( thisFence, eventHandle ) ) )
        {
            ReportError( "Failed to set completion event on fence." );
        }

        WaitForSingleObject( eventHandle, INFINITE );
        CloseHandle( eventHandle );
    }
}

std::shared_ptr<Buffer> RenderDeviceDX12::CreateFloatVertexBuffer( const float* data, unsigned int count, unsigned int stride )
{
    std::shared_ptr<Buffer> buffer = std::make_shared<BufferDX12>( m_pDevice.Get(), m_pCommandList.Get(), Buffer::VertexBuffer, data, count, stride );
    m_Buffers.push_back( buffer );

    return buffer;
}

std::shared_ptr<Buffer> RenderDeviceDX12::CreateDoubleVertexBuffer( const double* data, unsigned int count, unsigned int stride )
{
    std::shared_ptr<Buffer> buffer = std::make_shared<BufferDX12>( m_pDevice.Get(), m_pCommandList.Get(), Buffer::VertexBuffer, data, count, stride );
    m_Buffers.push_back( buffer );

    return buffer;
}

std::shared_ptr<Buffer> RenderDeviceDX12::CreateUIntIndexBuffer( const unsigned int* data, unsigned int count )
{
    std::shared_ptr<Buffer> pBuffer = std::make_shared<BufferDX12>( m_pDevice.Get(), m_pCommandList.Get(), Buffer::IndexBuffer, data, count, (UINT)sizeof( unsigned int ) );
    m_Buffers.push_back( pBuffer );

    return pBuffer;
}
void RenderDeviceDX12::DestroyBuffer( std::shared_ptr<Buffer> buffer )
{
    BufferList::iterator iter = std::find( m_Buffers.begin(), m_Buffers.end(), buffer );
    if ( iter != m_Buffers.end() )
    {
        m_Buffers.erase( iter );
    }
}

void RenderDeviceDX12::DestroyVertexBuffer( std::shared_ptr<Buffer> buffer )
{
    DestroyBuffer( buffer );
}

void RenderDeviceDX12::DestroyIndexBuffer( std::shared_ptr<Buffer> buffer )
{
    DestroyBuffer( buffer );
}

std::shared_ptr<ConstantBuffer> RenderDeviceDX12::CreateConstantBuffer( const void* data, unsigned int size )
{
    // ConstantBuffer* pBuffer = new ConstantBufferDX12( m_pDevice, size );
    // pBuffer->Set( data, size );
    // m_Buffers.push_back( pBuffer );

    return nullptr; // pBuffer;
}

void RenderDeviceDX12::DestroyConstantBuffer( std::shared_ptr<ConstantBuffer> buffer )
{
    DestroyBuffer( buffer );
}

std::shared_ptr<StructuredBuffer> RenderDeviceDX12::CreateStructuredBuffer( void* data, unsigned int count, unsigned int stride, CPUAccess cpuAccess, bool gpuWrite )
{
    //    StructuredBuffer* pBuffer = new StructuredBufferDX11( m_pDevice, 0, data, count, stride, cpuAccess );
    //    m_Buffers.push_back( pBuffer );

    return nullptr; // pBuffer;
}

void RenderDeviceDX12::DestroyStructuredBuffer( std::shared_ptr<StructuredBuffer> buffer )
{
    DestroyBuffer( buffer );
}

std::shared_ptr<Scene> RenderDeviceDX12::CreateScene()
{
    return nullptr;
}

void RenderDeviceDX12::DestroyScene( std::shared_ptr<Scene> scene )
{
    SceneList::iterator iter = std::find( m_Scenes.begin(), m_Scenes.end(), scene );
    if ( iter != m_Scenes.end() )
    {
        m_Scenes.erase( iter );
    }
}


std::shared_ptr<Mesh> RenderDeviceDX12::CreateMesh()
{
    //Mesh* pMesh = new MeshDX11( m_pDevice );
    //m_Meshs.push_back( pMesh );

    return nullptr; // pMesh;
}

void RenderDeviceDX12::DestroyMesh( std::shared_ptr<Mesh> mesh )
{
    MeshList::iterator iter = std::find( m_Meshs.begin(), m_Meshs.end(), mesh );
    if ( iter != m_Meshs.end() )
    {
        m_Meshs.erase( iter );
    }
}

std::shared_ptr<Shader> RenderDeviceDX12::CreateShader()
{
    //Shader* pShader = new ShaderDX11( m_pDevice );
    //m_Shaders.push_back( pShader );

    return nullptr; // pShader;
}

void RenderDeviceDX12::DestroyShader( std::shared_ptr<Shader> shader )
{
    ShaderList::iterator iter = std::find( m_Shaders.begin(), m_Shaders.end(), shader );
    if ( iter != m_Shaders.end() )
    {
        m_Shaders.erase( iter );
    }
}

std::shared_ptr<Texture> RenderDeviceDX12::CreateTexture( const std::string& fileName )
{
    return nullptr;
}

std::shared_ptr<Texture> RenderDeviceDX12::CreateTexture( uint16_t width, uint16_t height )
{
    //Texture* pTexture = new TextureDX11( m_pDevice, width, height );
    //m_Textures.push_back( pTexture );

    return nullptr; // pTexture;
}

void RenderDeviceDX12::DestroyTexture( std::shared_ptr<Texture> texture )
{
    TextureList::iterator iter = std::find( m_Textures.begin(), m_Textures.end(), texture );
    if ( iter != m_Textures.end() )
    {
        m_Textures.erase( iter );
    }
}

std::shared_ptr<SamplerState> RenderDeviceDX12::CreateSamplerState()
{
    //SamplerState* pSampler = new SamplerStateDX11( m_pDevice );
    //m_Samplers.push_back( pSampler );

    return nullptr; // pSampler;
}

void RenderDeviceDX12::DestroySampler( std::shared_ptr<SamplerState> sampler )
{
    SamplerList::iterator iter = std::find( m_Samplers.begin(), m_Samplers.end(), sampler );
    if ( iter != m_Samplers.end() )
    {
        m_Samplers.erase( iter );
    }
}