#pragma once

#include <RenderDevice.h>

class Application;

class RenderDeviceDX12 : public RenderDevice
{
public:
    typedef RenderDevice base;

    RenderDeviceDX12( Application& app );
    virtual ~RenderDeviceDX12();

    virtual std::shared_ptr<Buffer> CreateFloatVertexBuffer( const float* data, unsigned int count, unsigned int stride );
    virtual std::shared_ptr<Buffer> CreateDoubleVertexBuffer( const double* data, unsigned int count, unsigned int stride );
    virtual std::shared_ptr<Buffer> CreateUIntIndexBuffer( const unsigned int* data, unsigned int sizeInBytes );
    virtual std::shared_ptr<ConstantBuffer> CreateConstantBuffer( const void* data, unsigned int size );
    virtual std::shared_ptr<StructuredBuffer> CreateStructuredBuffer( void* data, unsigned int count, unsigned int stride, CPUAccess cpuAccess = CPUAccess::None, bool gpuWrite = false );

    virtual void DestroyBuffer( std::shared_ptr<Buffer> buffer );
    virtual void DestroyVertexBuffer( std::shared_ptr<Buffer> buffer );
    virtual void DestroyIndexBuffer( std::shared_ptr<Buffer> buffer );
    virtual void DestroyConstantBuffer( std::shared_ptr<ConstantBuffer> buffer );
    virtual void DestroyStructuredBuffer( std::shared_ptr<StructuredBuffer> buffer );

    virtual std::shared_ptr<Shader> CreateShader();
    virtual void DestroyShader( std::shared_ptr<Shader> shader );

    virtual std::shared_ptr<Scene> CreateScene();
    virtual void DestroyScene( std::shared_ptr<Scene> Model );

    virtual std::shared_ptr<Mesh> CreateMesh();
    virtual void DestroyMesh( std::shared_ptr<Mesh> mesh );

    virtual std::shared_ptr<Texture> CreateTexture( const std::string& fileName );
    virtual std::shared_ptr<Texture> CreateTexture( uint16_t width, uint16_t height );
    virtual void DestroyTexture( std::shared_ptr<Texture> texture );

    virtual std::shared_ptr<SamplerState> CreateSamplerState();
    virtual void DestroySampler( std::shared_ptr<SamplerState> sampler );

    virtual Microsoft::WRL::ComPtr<IDXGISwapChain> CreateSwapChain( const DXGI_SWAP_CHAIN_DESC& swapChainDesc );

    Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() const;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() const;

    void CommitCommandList();
    void CommitCommandListAndResetAllocator();

    void WaitForPreviousFrame();


protected:
    virtual void CreateDevice( HINSTANCE hInstance );

private:
    Application& m_App;
    Microsoft::WRL::ComPtr<IDXGIFactory4> m_pFactory;
    Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;

    // Default shaders used for creating the pipeline state object.
    Microsoft::WRL::ComPtr<ID3DBlob> m_pVertexShaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> m_pPixelShaderBlob;
    D3D12_SHADER_BYTECODE m_VertexShaderByteCode;
    D3D12_SHADER_BYTECODE m_PixelShaderByteCode;

    // Default Pipeline State Object.
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;
    // Default command list.
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
    // Command queue required to execute command lists.
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
    // Empty root signature.
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
    // Fence object for GPU synchronization
    Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
    UINT64 m_uiCurrentFence;

    // An event handle to use for frame synchronization.
    HANDLE m_FrameEventHandle;

    typedef std::vector< std::shared_ptr<Buffer> > BufferList;
    BufferList m_Buffers;

    typedef std::vector< std::shared_ptr<Scene> > SceneList;
    SceneList m_Scenes;

    typedef std::vector< std::shared_ptr<Mesh> > MeshList;
    MeshList m_Meshs;

    typedef std::vector< std::shared_ptr<Shader> > ShaderList;
    ShaderList m_Shaders;

    typedef std::vector< std::shared_ptr<Texture> > TextureList;
    TextureList m_Textures;

    typedef std::vector< std::shared_ptr<SamplerState> > SamplerList;
    SamplerList m_Samplers;

};