#pragma once

#include "Object.h"
#include "Events.h"
#include "Texture.h"
#include "Query.h"
#include "CPUAccess.h"

class RenderWindow;
class Buffer;
class ConstantBuffer;
class StructuredBuffer;
class Scene;
class Mesh;
class Shader;
class SamplerState;
class Material;
class PipelineState;
class RenderTarget;
// class Query;

/**
 * Render device encapsulates functions for creating resources on the GPU.
 */
class RenderDevice : public Object
{
public:
    // Register for this event to receive scene loading progress.    
    ProgressEvent LoadingProgress;

    // Get the name of the primary graphics device.
    virtual const std::string& GetDeviceName() const = 0;

    // Create an vertex buffer.
    template<typename T>
    std::shared_ptr<Buffer> CreateVertexBuffer( const T& data );
    virtual void DestroyVertexBuffer( std::shared_ptr<Buffer> buffer ) = 0;

    // Create an index buffer.
    template<typename T>
    std::shared_ptr<Buffer> CreateIndexBuffer( const T& data );
    virtual void DestroyIndexBuffer( std::shared_ptr<Buffer> buffer ) = 0;

    // Create a constant buffer (or Uniform buffer)
    template<typename T>
    std::shared_ptr<ConstantBuffer> CreateConstantBuffer( const T& data );
    virtual void DestroyConstantBuffer( std::shared_ptr<ConstantBuffer> buffer ) = 0;

    // Create a StructuredBuffer
    template<typename T>
    std::shared_ptr<StructuredBuffer> CreateStructuredBuffer( const std::vector<T>& data, CPUAccess cpuAccess = CPUAccess::None, bool gpuWrite = false );
    virtual void DestroyStructuredBuffer( std::shared_ptr<StructuredBuffer> buffer ) = 0;

    virtual std::shared_ptr<Scene> CreateScene() = 0;

    // Create a plane in 3D.
    // The plane will be centered at the origin.
    // @param size The size of the plane.
    // @param N Surface normal to the plane.
    virtual std::shared_ptr<Scene> CreatePlane( float size, const glm::vec3& N = glm::vec3( 0, 1, 0 ) ) = 0;
    
    // Create a screen-space quad that can be used to render full-screen post-process effects to the screen.
    // By default, the quad will have clip-space coordinates and can be used with a pass-through vertex shader
    // to render full-screen post-process effects. If you want more control over the area of the screen the quad covers, 
    // you can specify your own screen coordinates and supply an appropriate orthographic projection matrix to align the 
    // screen quad appropriately.
    virtual std::shared_ptr<Scene> CreateScreenQuad( float left = -1.0f, float right = 1.0f, float bottom = -1.0f, float top = 1.0f, float z = 0.0f ) = 0;

    // Create a sphere in 3D
    // @param radius Radius of the sphere.
    // @param tesselation The amount of tessellation to apply to the sphere. Default tessellation is 4.
    virtual std::shared_ptr<Scene> CreateSphere( float radius, float tesselation = 4 ) = 0;
    
    // Create a cube in 3D.
    // The cube will be centered at the origin.
    // @param size The length of each edge of the cube.
    virtual std::shared_ptr<Scene> CreateCube( float size ) = 0;
    
    // Create a cylinder that is aligned to a particular axis.
    // @param baseRadius The radius of the base (bottom) of the cylinder.
    // @param apexRadius The radius of the apex (top) of the cylinder.
    // @param height The height of the sphere along the axis of the cylinder.
    // @param axis The axis to align the cylinder. Default to the global Y axis.
    virtual std::shared_ptr<Scene> CreateCylinder( float baseRadius, float apexRadius, float height, const glm::vec3& axis = glm::vec3( 0, 1, 0 ) ) = 0;
    
    // Create a cone.
    // Cones are always aligned to (0, 1, 0) with the base of the cone 
    // centered at (0, 0, 0) and apex at (0, height, 0).
    // A cone is just a cylinder with an apex radius of 0.
    // @param baseRadius The radius of the base of the cone.
    // @param height The height of the cone.
    virtual std::shared_ptr<Scene> CreateCone( float baseRadius, float height ) = 0;
    
    // Create a 3D arrow.
    // Arrows can be used to represent the direction an object or light is pointing.
    // @param tail The tail (begin point) of the arrow.
    // @param head The head (end point) of the arrow.
    // @param radius The radius of the body of the arrow.
    virtual std::shared_ptr<Scene> CreateArrow( const glm::vec3& tail = glm::vec3( 0, 0, 0), const glm::vec3& head = glm::vec3( 0, 0, 1 ), float radius = 0.05f ) = 0;

    // Create a 3D axis with X, -X, Y, -Y, Z, -Z axes.
    // Primarily used to debug an object's position and direction in 3D space.
    // The axis is aligned to 0,0,0 and the global X, Y, Z axes.
    // @param radius is the radius of the axis arms.
    // @param length is the length is the length of each axis arm.
    virtual std::shared_ptr<Scene> CreateAxis( float radius = 0.05f, float length = 0.5f ) = 0;

    virtual void DestroyScene( std::shared_ptr<Scene> scene ) = 0;

    virtual std::shared_ptr<Mesh> CreateMesh() = 0;
    virtual void DestroyMesh( std::shared_ptr<Mesh> mesh ) = 0;

    virtual std::shared_ptr<Shader> CreateShader() = 0;
    virtual void DestroyShader( std::shared_ptr<Shader> shader ) = 0;

    // Create a texture from a file.
    virtual std::shared_ptr<Texture> CreateTexture( const std::wstring& fileName ) = 0;
    virtual std::shared_ptr<Texture> CreateTextureCube( const std::wstring& fileName ) = 0;

    // Create an empty texture of a predefined size.
    virtual std::shared_ptr<Texture> CreateTexture1D( uint16_t width, uint16_t slices, const Texture::TextureFormat& format = Texture::TextureFormat(), CPUAccess cpuAccess = CPUAccess::None, bool gpuWrite = false ) = 0;
    virtual std::shared_ptr<Texture> CreateTexture2D( uint16_t width, uint16_t height, uint16_t slices, const Texture::TextureFormat& format = Texture::TextureFormat(), CPUAccess cpuAccess = CPUAccess::None, bool gpuWrite = false ) = 0;
    virtual std::shared_ptr<Texture> CreateTexture3D( uint16_t width, uint16_t height, uint16_t depth, const Texture::TextureFormat& format = Texture::TextureFormat(), CPUAccess cpuAccess = CPUAccess::None, bool gpuWrite = false ) = 0;
    virtual std::shared_ptr<Texture> CreateTextureCube( uint16_t size, uint16_t numCubes = 1, const Texture::TextureFormat& format = Texture::TextureFormat(), CPUAccess cpuAccess = CPUAccess::None, bool gpuWrite = false ) = 0;

    // Create an null texture (can be loaded later using Texture::LoadTexture2D function.
    virtual std::shared_ptr<Texture> CreateTexture() = 0;

    // Release a texture.
    virtual void DestroyTexture( std::shared_ptr<Texture> texture ) = 0;

    // Create a render target
    virtual std::shared_ptr<RenderTarget> CreateRenderTarget() = 0;
    virtual void DestroyRenderTarget( std::shared_ptr<RenderTarget> renderTarget ) = 0;

    // Create a GPU query object. Used for performance profiling, occlusion queries,
    // or primitive output queries.
    virtual std::shared_ptr<Query> CreateQuery( Query::QueryType queryType = Query::QueryType::Timer, uint8_t numBuffers = 3 ) = 0;
    virtual void DestoryQuery( std::shared_ptr<Query> query ) = 0;


    virtual std::shared_ptr<SamplerState> CreateSamplerState() = 0;
    virtual void DestroySampler( std::shared_ptr<SamplerState> sampler ) = 0;

    virtual std::shared_ptr<Material> CreateMaterial() = 0;
    virtual void DestroyMaterial( std::shared_ptr<Material> material ) = 0;

    virtual std::shared_ptr<PipelineState> CreatePipelineState() = 0;
    virtual void DestoryPipelineState( std::shared_ptr<PipelineState> pipeline ) = 0;

    virtual std::shared_ptr<Buffer> CreateFloatVertexBuffer( const float* data, unsigned int count, unsigned int stride ) = 0;
    virtual std::shared_ptr<Buffer> CreateDoubleVertexBuffer( const double* data, unsigned int count, unsigned int stride ) = 0;
    virtual std::shared_ptr<Buffer> CreateUIntIndexBuffer( const unsigned int* data, unsigned int sizeInBytes ) = 0;
    virtual std::shared_ptr<ConstantBuffer> CreateConstantBuffer( const void* data, size_t size ) = 0;
    virtual std::shared_ptr<StructuredBuffer> CreateStructuredBuffer( void* data, unsigned int count, unsigned int stride, CPUAccess cpuAccess = CPUAccess::None, bool gpuWrite = false ) = 0;

protected: 
    virtual void OnLoadingProgress( ProgressEventArgs& e );
};

#include "RenderDevice.inl"