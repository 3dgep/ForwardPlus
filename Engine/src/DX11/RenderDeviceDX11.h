#pragma once

#include <RenderDevice.h>

class Application;
class Material;

class RenderDeviceDX11 : public RenderDevice
{
public:
    typedef RenderDevice base;

    RenderDeviceDX11( Application& app );
    virtual ~RenderDeviceDX11();

    virtual const std::string& GetDeviceName() const;
    
    // Inherited from RenderDevice
    virtual std::shared_ptr<Buffer> CreateFloatVertexBuffer( const float* data, unsigned int count, unsigned int stride );
    virtual std::shared_ptr<Buffer> CreateDoubleVertexBuffer( const double* data, unsigned int count, unsigned int stride );
    virtual std::shared_ptr<Buffer> CreateUIntIndexBuffer( const unsigned int* data, unsigned int count );
    virtual std::shared_ptr<ConstantBuffer> CreateConstantBuffer( const void* data, size_t size );
    virtual std::shared_ptr<StructuredBuffer> CreateStructuredBuffer( void* data, unsigned int count, unsigned int stride, CPUAccess cpuAccess = CPUAccess::None, bool gpuWrite = false );

    virtual void DestroyBuffer( std::shared_ptr<Buffer> buffer );
    virtual void DestroyVertexBuffer( std::shared_ptr<Buffer> buffer );
    virtual void DestroyIndexBuffer( std::shared_ptr<Buffer> buffer );
    virtual void DestroyConstantBuffer( std::shared_ptr<ConstantBuffer> buffer );
    virtual void DestroyStructuredBuffer( std::shared_ptr<StructuredBuffer> buffer );

    virtual std::shared_ptr<Shader> CreateShader();
    virtual void DestroyShader( std::shared_ptr<Shader> shader );

    virtual std::shared_ptr<Scene> CreateScene();
    virtual std::shared_ptr<Scene> CreatePlane( float size, const glm::vec3& N = glm::vec3( 0, 1, 0 ) );
    virtual std::shared_ptr<Scene> CreateScreenQuad( float left = -1.0f, float right = 1.0f, float bottom = -1.0f, float top = 1.0f, float z = 0.0f );
    virtual std::shared_ptr<Scene> CreateSphere( float radius, float tesselation = 4 );
    virtual std::shared_ptr<Scene> CreateCube( float size );
    virtual std::shared_ptr<Scene> CreateCylinder( float baseRadius, float apexRadius, float height, const glm::vec3& axis = glm::vec3( 0, 1, 0 ) );
    virtual std::shared_ptr<Scene> CreateCone( float baseRadius, float height );
    virtual std::shared_ptr<Scene> CreateArrow( const glm::vec3& tail = glm::vec3( 0, 0, 0 ), const glm::vec3& head = glm::vec3( 0, 0, 1 ), float radius = 0.05f );
    virtual std::shared_ptr<Scene> CreateAxis( float radius = 0.05f, float length = 0.5f );
    virtual void DestroyScene( std::shared_ptr<Scene> scene );

    virtual std::shared_ptr<Mesh> CreateMesh();
    virtual void DestroyMesh( std::shared_ptr<Mesh> mesh );

    virtual std::shared_ptr<Texture> CreateTexture( const std::wstring& fileName );
    virtual std::shared_ptr<Texture> CreateTextureCube( const std::wstring& fileName );

    virtual std::shared_ptr<Texture> CreateTexture1D( uint16_t width, uint16_t slices = 1, const Texture::TextureFormat& format = Texture::TextureFormat(), CPUAccess cpuAccess = CPUAccess::None, bool gpuWrite = false );
    virtual std::shared_ptr<Texture> CreateTexture2D( uint16_t width, uint16_t height, uint16_t slices = 1, const Texture::TextureFormat& format = Texture::TextureFormat(), CPUAccess cpuAccess = CPUAccess::None, bool gpuWrite = false );
    virtual std::shared_ptr<Texture> CreateTexture3D( uint16_t width, uint16_t height, uint16_t depth, const Texture::TextureFormat& format = Texture::TextureFormat(), CPUAccess cpuAccess = CPUAccess::None, bool gpuWrite = false );
    virtual std::shared_ptr<Texture> CreateTextureCube( uint16_t size, uint16_t numCubes = 1, const Texture::TextureFormat& format = Texture::TextureFormat(), CPUAccess cpuAccess = CPUAccess::None, bool gpuWrite = false );
    virtual std::shared_ptr<Texture> CreateTexture();
    virtual std::shared_ptr<Texture> GetDefaultTexture() const;

    virtual void DestroyTexture( std::shared_ptr<Texture> texture );

    virtual std::shared_ptr<Query> CreateQuery( Query::QueryType queryType = Query::QueryType::Timer, uint8_t numBuffers = 3 );
    virtual void DestoryQuery( std::shared_ptr<Query> query );

    // Create a render target
    virtual std::shared_ptr<RenderTarget> CreateRenderTarget();
    virtual void DestroyRenderTarget( std::shared_ptr<RenderTarget> renderTarget );

    virtual std::shared_ptr<SamplerState> CreateSamplerState();
    virtual void DestroySampler( std::shared_ptr<SamplerState> sampler );

    virtual std::shared_ptr<Material> CreateMaterial();
    virtual void DestroyMaterial( std::shared_ptr<Material> Material );

    virtual std::shared_ptr<PipelineState> CreatePipelineState();
    virtual void DestoryPipelineState( std::shared_ptr<PipelineState> pipeline );

    // Specific to RenderDeviceDX11
    Microsoft::WRL::ComPtr<ID3D11Device2> GetDevice() const;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext2> GetDeviceContext() const;

protected:
    virtual void CreateDevice( HINSTANCE hInstance );

    // Called when the application is initialized.
    virtual void OnInitialize( EventArgs& e );
    virtual void OnLoadingProgress( ProgressEventArgs& e );

private:
    Microsoft::WRL::ComPtr<ID3D11Device2> m_pDevice;
    Microsoft::WRL::ComPtr<ID3D11Debug> m_pDebugLayer;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext2> m_pDeviceContext;

    // The name of the graphics device used for rendering.
    std::string m_DeviceName;

    typedef std::vector< std::shared_ptr<Scene> > SceneList;
    SceneList m_Scenes;

    typedef std::vector< std::shared_ptr<Buffer> > BufferList;
    BufferList m_Buffers;

    typedef std::vector< std::shared_ptr<Mesh> > MeshList;
    MeshList m_Meshes;

    typedef std::vector< std::shared_ptr<Shader> > ShaderList;
    ShaderList m_Shaders;

    typedef std::vector< std::shared_ptr<Texture> > TextureList;
    typedef std::map< std::wstring, std::shared_ptr<Texture> > TextureMap;
    TextureList m_Textures;
    TextureMap m_TexturesByName;

    typedef std::vector< std::shared_ptr<RenderTarget> > RenderTargetList;
    RenderTargetList m_RenderTargets;

    std::shared_ptr<Texture> m_pDefaultTexture;

    typedef std::vector< std::shared_ptr<SamplerState> > SamplerList;
    SamplerList m_Samplers;

    typedef std::vector< std::shared_ptr<Material> > MaterialList;
    MaterialList m_Materials;

    typedef std::vector< std::shared_ptr<PipelineState> > PipelineList;
    PipelineList m_Pipelines;

    typedef std::vector< std::shared_ptr<Query> > QueryList;
    QueryList m_Queries;

    std::shared_ptr<PipelineState> m_pDefaultPipeline;

    void LoadDefaultResources();
};