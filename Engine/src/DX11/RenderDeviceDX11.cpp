#include <EnginePCH.h>

#include "../resource.h"

#include <Application.h>
#include <Material.h>

#include "BufferDX11.h"
#include "ConstantBufferDX11.h"
#include "StructuredBufferDX11.h"
#include "RenderTargetDX11.h"
#include "MeshDX11.h"
#include "SceneDX11.h"
#include "ShaderDX11.h"
#include "TextureDX11.h"
#include "SamplerStateDX11.h"
#include "PipelineStateDX11.h"
#include "QueryDX11.h"

#include "RenderDeviceDX11.h"

using Microsoft::WRL::ComPtr;

RenderDeviceDX11::RenderDeviceDX11( Application& app )
{
    CreateDevice( app.GetModuleHandle() );

    app.Initialize += boost::bind( &RenderDeviceDX11::OnInitialize, this, _1 );

    // Initialize AntTweak bar.
    TwInit( TW_DIRECT3D11, m_pDevice.Get() );
}

RenderDeviceDX11::~RenderDeviceDX11()
{
    TwTerminate();

    m_Materials.clear();
    m_Scenes.clear();
    m_Meshes.clear();
    m_Buffers.clear();
    m_Shaders.clear();
    m_Textures.clear();
    m_Samplers.clear();
    m_Pipelines.clear();
    m_Queries.clear();

#if defined(_DEBUG)
    if ( m_pDebugLayer )
    {
//        m_pDebugLayer->ReportLiveDeviceObjects( D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL );
    }
#endif
}

void RenderDeviceDX11::CreateDevice( HINSTANCE hInstance )
{
    const D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
    };

    UINT createDeviceFlags = 0;
#if defined(_DEBUG)
    createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif

    // This will be the feature level that 
    // is used to create our device and swap chain.
    D3D_FEATURE_LEVEL featureLevel;

    Microsoft::WRL::ComPtr<ID3D11Device> pDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> pDeviceContext;

    // First create a ID3D11Device and ID3D11DeviceContext
    HRESULT hr = D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, createDeviceFlags, featureLevels, _countof( featureLevels ),
        D3D11_SDK_VERSION, &pDevice, &featureLevel, &pDeviceContext );

    if ( hr == E_INVALIDARG )
    {
        hr = D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE,
            nullptr, createDeviceFlags, &featureLevels[1], _countof( featureLevels ) - 1,
            D3D11_SDK_VERSION, &pDevice, &featureLevel, &pDeviceContext );
    }

    if ( FAILED( hr ) )
    {
        ReportError( "Failed to created DirectX 11 Device" );
        return;
    }

    // Now query for the ID3D11Device2 interface.
    if ( FAILED( pDevice.Get()->QueryInterface<ID3D11Device2>( &m_pDevice ) ) )
    {
        ReportError( "Failed to create DirectX 11.2 device" );
    }

    // Now get the immediate device context.
    m_pDevice->GetImmediateContext2( &m_pDeviceContext );

    if ( SUCCEEDED( m_pDevice.Get()->QueryInterface<ID3D11Debug>( &m_pDebugLayer ) ) )
    {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        if ( SUCCEEDED( m_pDebugLayer.Get()->QueryInterface<ID3D11InfoQueue>( &d3dInfoQueue ) ) )
        {
#if defined(_DEBUG)
            d3dInfoQueue->SetBreakOnSeverity( D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE );
            d3dInfoQueue->SetBreakOnSeverity( D3D11_MESSAGE_SEVERITY_ERROR, TRUE );
            d3dInfoQueue->SetBreakOnSeverity( D3D11_MESSAGE_SEVERITY_WARNING, TRUE );
#endif 
            D3D11_MESSAGE_ID hide[] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                D3D11_MESSAGE_ID_QUERY_BEGIN_ABANDONING_PREVIOUS_RESULTS,
                D3D11_MESSAGE_ID_QUERY_END_ABANDONING_PREVIOUS_RESULTS,
                // Add more message IDs here as needed
            };

            D3D11_INFO_QUEUE_FILTER filter;
            memset( &filter, 0, sizeof( filter ) );
            filter.DenyList.NumIDs = _countof( hide );
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries( &filter );
        }
    }

    // Query the adapter information.
    Microsoft::WRL::ComPtr<IDXGIFactory> factory;
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    DXGI_ADAPTER_DESC adapterDescription = {};

    if ( FAILED( CreateDXGIFactory( __uuidof( IDXGIFactory ), &factory ) ) )
    {
        ReportError( "Failed to create DXGIFactory." );
    }
    if ( FAILED( factory->EnumAdapters( 0, &adapter ) ) )
    {
        ReportError( "Failed to enumerate adapter." );
    }
    if ( FAILED( adapter->GetDesc( &adapterDescription ) ) )
    {
        ReportError( "Failed to qauery adapter description." );
    }

    m_DeviceName = ConvertString( adapterDescription.Description );

}

const std::string& RenderDeviceDX11::GetDeviceName() const
{
    return m_DeviceName;
}

Microsoft::WRL::ComPtr<ID3D11Device2> RenderDeviceDX11::GetDevice() const
{
    return m_pDevice;
}

Microsoft::WRL::ComPtr<ID3D11DeviceContext2> RenderDeviceDX11::GetDeviceContext() const
{
    return m_pDeviceContext;
}


std::shared_ptr<Buffer> RenderDeviceDX11::CreateFloatVertexBuffer( const float* data, unsigned int count, unsigned int stride )
{
    std::shared_ptr<Buffer> buffer = std::make_shared<BufferDX11>( m_pDevice.Get(), D3D11_BIND_VERTEX_BUFFER, data, count, stride );
    m_Buffers.push_back( buffer );

    return buffer;
}

std::shared_ptr <Buffer> RenderDeviceDX11::CreateDoubleVertexBuffer( const double* data, unsigned int count, unsigned int stride )
{
    std::shared_ptr<Buffer> buffer = std::make_shared<BufferDX11>( m_pDevice.Get(), D3D11_BIND_VERTEX_BUFFER, data, count, stride );
    m_Buffers.push_back( buffer );

    return buffer;
}

std::shared_ptr<Buffer> RenderDeviceDX11::CreateUIntIndexBuffer( const unsigned int* data, unsigned int count )
{
    std::shared_ptr <Buffer> buffer = std::make_shared<BufferDX11>( m_pDevice.Get(), D3D11_BIND_INDEX_BUFFER, data, count, (UINT)sizeof( unsigned int ) );
    m_Buffers.push_back( buffer );

    return buffer;
}

void RenderDeviceDX11::DestroyBuffer( std::shared_ptr<Buffer> buffer )
{
    BufferList::iterator iter = std::find( m_Buffers.begin(), m_Buffers.end(), buffer );
    if ( iter != m_Buffers.end() )
    {
        m_Buffers.erase( iter );
    }
}

void RenderDeviceDX11::DestroyVertexBuffer( std::shared_ptr<Buffer> buffer )
{
    DestroyBuffer( buffer );
}

void RenderDeviceDX11::DestroyIndexBuffer( std::shared_ptr<Buffer> buffer )
{
    DestroyBuffer( buffer );
}

std::shared_ptr<ConstantBuffer> RenderDeviceDX11::CreateConstantBuffer( const void* data, size_t size )
{
    std::shared_ptr<ConstantBuffer> buffer = std::make_shared<ConstantBufferDX11>( m_pDevice.Get(), size );

    if ( data )
    {
        buffer->Set( data, size );
    }

    m_Buffers.push_back( buffer );

    return buffer;
}

void RenderDeviceDX11::DestroyConstantBuffer( std::shared_ptr<ConstantBuffer> buffer )
{
    DestroyBuffer( buffer );
}

std::shared_ptr<StructuredBuffer> RenderDeviceDX11::CreateStructuredBuffer( void* data, unsigned int count, unsigned int stride, CPUAccess cpuAccess, bool gpuWrite )
{
    std::shared_ptr<StructuredBuffer> buffer = std::make_shared<StructuredBufferDX11>( m_pDevice.Get(), 0, data, count, stride, cpuAccess, gpuWrite );
    m_Buffers.push_back( buffer );

    return buffer;
}

void RenderDeviceDX11::DestroyStructuredBuffer( std::shared_ptr<StructuredBuffer> buffer )
{
    DestroyBuffer( buffer );
}

std::shared_ptr<Mesh> RenderDeviceDX11::CreateMesh()
{
    std::shared_ptr<Mesh> mesh = std::make_shared<MeshDX11>( m_pDevice.Get() );
    m_Meshes.push_back( mesh );

    return mesh;
}

void RenderDeviceDX11::DestroyMesh( std::shared_ptr<Mesh> mesh )
{
    MeshList::iterator iter = std::find( m_Meshes.begin(), m_Meshes.end(), mesh );
    if ( iter != m_Meshes.end() )
    {
        m_Meshes.erase( iter );
    }
}

std::shared_ptr<Scene> RenderDeviceDX11::CreateScene()
{
    std::shared_ptr<Scene> scene = std::make_shared<SceneDX11>( *this );
    scene->LoadingProgress += boost::bind( &RenderDeviceDX11::OnLoadingProgress, this, _1 );

    m_Scenes.push_back( scene );

    return scene;
}

// GLM's own quaternion from two vector constructor does not handle cases 
// where the vectors may be pointing in opposite directions.
// This method handles the cases where the u and v vectors are opposites.
// source: http://lolengine.net/blog/2014/02/24/quaternion-from-two-vectors-final
// accessed: 26/05/2015
inline glm::quat RotationFromTwoVectors( const glm::vec3& u, const glm::vec3& v )
{
    float normUV = glm::sqrt( glm::dot( u, u ) * glm::dot( v, v ) );
    float real = normUV + glm::dot( u, v );

    glm::vec3 vec;

    if ( real < 1.e-6f * normUV )
    {
        /* If u and v are exactly opposite, rotate 180 degrees
         * around an arbitrary orthogonal axis. Axis normalisation
         * can happen later, when we normalise the quaternion. 
         */
        real = 0.0f;
        vec = ( glm::abs( u.x ) > abs( u.z ) ) ? glm::vec3( -u.y, u.x, 0.0f ) : glm::vec3( 0.0f, -u.z, u.y );
    }
    else
    {
        /* Otherwise, build quaternion the standard way. */
        vec = glm::cross( u, v );
    }

    return glm::normalize( glm::quat( real, vec ) );
}

std::shared_ptr<Scene> RenderDeviceDX11::CreatePlane( float size, const glm::vec3& N )
{
    float halfSize = size * 0.5f;
    glm::vec3 p[4];
    // Crate the 4 points of the plane aligned to the X,Z plane.
    // Vertex winding is assuming a right-handed coordinate system 
    // (counter-clockwise winding order for front-facing polygons)
    p[0] = glm::vec3(  halfSize, 0,  halfSize );
    p[1] = glm::vec3( -halfSize, 0,  halfSize );
    p[2] = glm::vec3( -halfSize, 0, -halfSize );
    p[3] = glm::vec3(  halfSize, 0, -halfSize );

    // Rotate the plane vertices in the direction of the surface normal.
    glm::quat rot = RotationFromTwoVectors( glm::vec3( 0, 1, 0 ), N );

    for ( int i = 0; i < 4; i++ )
    {
        p[i] = rot * p[i];
    }

    // Now create the plane polygon from the transformed vertices.
    std::shared_ptr<Scene> scene = CreateScene();

    std::stringstream ss;

    // Create a white diffuse material for the plane.
    // f red green blue Kd Ks Shine transmittance indexOfRefraction
    ss << "f 1 1 1 1 0 0 0 0" << std::endl;

    // Create a 4-point polygon
    ss << "p 4" << std::endl;
    for ( int i = 0; i < 4; i++ )
    {
        ss << p[i].x << " " << p[i].y << " " << p[i].z << std::endl;
    }

    if ( scene->LoadFromString( ss.str(), "nff" ) )
    {
        return scene;
    }

    // An error occurred while loading the scene.
    DestroyScene( scene );
    return nullptr;
}

std::shared_ptr<Scene> RenderDeviceDX11::CreateScreenQuad( float left, float right, float bottom, float top, float z )
{
    glm::vec3 p[4]; // Vertex position
    glm::vec3 n[4]; // Vertex normal (required for texture patch polygons)
    glm::vec2 t[4]; // Texture coordinates
    // Winding order is assumed to be right-handed. Front-facing polygons have
    // a counter-clockwise winding order.
    // Assimp flips the winding order of vertices.. Don't ask me why. To account for this,
    // the vertices are loaded in reverse order :)
    p[0] = glm::vec3( right, bottom, z );   n[0] = glm::vec3( 0, 0, 1 );    t[0] = glm::vec2( 1, 0 );
    p[1] = glm::vec3( left, bottom, z );    n[1] = glm::vec3( 0, 0, 1 );    t[1] = glm::vec2( 0, 0 );
    p[2] = glm::vec3( left, top, z );       n[2] = glm::vec3( 0, 0, 1 );    t[2] = glm::vec2( 0, 1 );
    p[3] = glm::vec3( right, top, z );      n[3] = glm::vec3( 0, 0, 1 );    t[3] = glm::vec2( 1, 1 );

    // Now create the quad.
    std::shared_ptr<Scene> scene = CreateScene();

    std::stringstream ss;

    // Create a white diffuse material for the quad.
    // f red green blue Kd Ks Shine transmittance indexOfRefraction
    ss << "f 1 1 1 1 0 0 0 0" << std::endl;

    // Create a 4-point textured polygon patch
    ss << "tpp 4" << std::endl;
    for ( int i = 0; i < 4; i++ )
    {
        // px py pz nx ny nz tu tv
        ss << p[i].x << " " << p[i].y << " " << p[i].z << " " << n[i].x << " " << n[i].y << " " << n[i].z << " " << t[i].x << " " << t[i].y << std::endl;
    }

    if ( scene->LoadFromString( ss.str(), "nff" ) )
    {
        return scene;
    }

    // An error occurred while loading the scene.
    DestroyScene( scene );
    return nullptr;

}

std::shared_ptr<Scene> RenderDeviceDX11::CreateSphere( float radius, float tesselation )
{
    std::shared_ptr<Scene> scene = CreateScene();
    std::stringstream ss;
    // Create a white diffuse material for the sphere.
    // f red green blue Kd Ks Shine transmittance indexOfRefraction
    ss << "f 1 1 1 1 0 0 0 0" << std::endl;

    // tess tesselation
    ss << "tess " << tesselation << std::endl;
    // s x y z radius
    ss << "s 0 0 0 " << radius << std::endl;

    if ( scene->LoadFromString( ss.str(), "nff" ) )
    {
        return scene;
    }

    // An error occurred while loading the scene.
    DestroyScene( scene );
    return nullptr;
}

std::shared_ptr<Scene> RenderDeviceDX11::CreateCube( float size )
{
    std::shared_ptr<Scene> scene = CreateScene();
    std::stringstream ss;

    // Create a white diffuse material for the cube.
    // f red green blue Kd Ks Shine transmittance indexOfRefraction
    ss << "f 1 1 1 1 0 0 0 0" << std::endl;

    // hex x y z size
    ss << "hex 0 0 0 " << size;

    if ( scene->LoadFromString( ss.str(), "nff" ) )
    {
        return scene;
    }

    // An error occurred while loading the scene.
    DestroyScene( scene );
    return nullptr;
}

std::shared_ptr<Scene> RenderDeviceDX11::CreateCylinder( float baseRadius, float apexRadius, float height, const glm::vec3& axis )
{
    std::shared_ptr<Scene> scene = CreateScene();
    std::stringstream ss;

    // Create a white diffuse material for the cylinder.
    // f red green blue Kd Ks Shine transmittance indexOfRefraction
    ss << "f 1 1 1 1 0 0 0 0" << std::endl;

    ss << "c" << std::endl;
    // base.x base.y base.z baseRadius
    ss << "0 0 0 " << baseRadius << std::endl;

    glm::vec3 apex = axis * height;
    // apex.x apex.y apex.z apexRadius
    ss << apex.x << " " << apex.y << " " << apex.z << " " << apexRadius << std::endl;

    if ( scene->LoadFromString( ss.str(), "nff" ) )
    {
        return scene;
    }

    // An error occurred while loading the scene.
    DestroyScene( scene );
    return nullptr;
}

std::shared_ptr <Scene> RenderDeviceDX11::CreateCone( float baseRadius, float height )
{
    // A cone is just a cylinder with a 0 size apex.
    return CreateCylinder( baseRadius, 0, height );
}

std::shared_ptr<Scene> RenderDeviceDX11::CreateArrow( const glm::vec3& tail, const glm::vec3& head, float radius )
{
    std::shared_ptr<Scene> scene = CreateScene();
    std::stringstream ss;

    glm::vec3 dir = head - tail;
    glm::vec3 apex = head + ( dir * 0.5f );

    // Create a white diffuse material for the arrow.
    // f red green blue Kd Ks Shine transmittance indexOfRefraction
    ss << "f 1 1 1 1 0 0 0 0" << std::endl;

    // Create a cylinder for the arrow body.
    ss << "c" << std::endl;
    // base.x base.y base.z baseRadius
    ss << tail.x << " " << tail.y << " " << tail.z << " " << radius << std::endl;
    // apex.x apex.y apex.z apexRadius
    ss << head.x << " " << head.y << " " << head.z << " " << radius << std::endl;

    // Create a cone for the arrow head.
    ss << "c" << std::endl;
    // base.x base.y base.z baseRadius
    ss << head.x << " " << head.y << " " << head.z << " " << radius * 2.0f << std::endl;

    // apex.x apex.y apex.z apexRadius
    ss << apex.x << " " << apex.y << " " << apex.z << " 0" << std::endl;

    if ( scene->LoadFromString( ss.str(), "nff" ) )
    {
        return scene;
    }

    // An error occurred while loading the scene.
    DestroyScene( scene );
    return nullptr;

}

std::shared_ptr<Scene> RenderDeviceDX11::CreateAxis( float radius, float length )
{
    std::shared_ptr<Scene> scene = CreateScene();
    std::stringstream ss;

    // Create a red material for the +X axis.
    // f red green blue Kd Ks Shine transmittance indexOfRefraction
    ss << "f 1 0 0 1 0 0 0 0" << std::endl;

    // Create a cylinder aligned to the +X axis.
    ss << "c" << std::endl;
    // base.x base.y base.z baseRadius
    ss << "0 0 0 " << radius << std::endl;
    // apex.x apex.y apex.z apexRadius
    ss << length << " 0 0 " << radius << std::endl;

    // Create a cone for the +X axis.
    ss << "c" << std::endl;
    // base.x base.y base.z baseRadius
    ss << length << " 0 0 " << radius * 2.0f << std::endl;
    // apex.x apex.y apex.z apexRadius
    ss << length * 1.5f << " 0 0 0" << std::endl;

    // Create a green material for the +Y axis.
    // f red green blue Kd Ks Shine transmittance indexOfRefraction
    ss << "f 0 1 0 1 0 0 0 0" << std::endl;

    // Create a cylinder aligned to the +Y axis.
    ss << "c" << std::endl;
    // base.x base.y base.z baseRadius
    ss << "0 0 0 " << radius << std::endl;
    // apex.x apex.y apex.z apexRadius
    ss << "0 " << length << " 0 " << radius << std::endl;

    // Create a cone for the +Y axis.
    ss << "c" << std::endl;
    // base.x base.y base.z baseRadius
    ss << "0 " << length << " 0 " << radius * 2.0f << std::endl;
    // apex.x apex.y apex.z apexRadius
    ss << "0 " << length * 1.5f << " 0 0" << std::endl;

    // Create a blue material for the +Z axis.
    // f red green blue Kd Ks Shine transmittance indexOfRefraction
    ss << "f 0 0 1 1 0 0 0 0" << std::endl;

    // Create a cylinder aligned to the +Z axis.
    ss << "c" << std::endl;
    // base.x base.y base.z baseRadius
    ss << "0 0 0 " << radius << std::endl;
    // apex.x apex.y apex.z apexRadius
    ss << "0 0 " << length << " " << radius << std::endl;

    // Create a cone for the +Z axis.
    ss << "c" << std::endl;
    // base.x base.y base.z baseRadius
    ss << "0 0 " << length << " " << radius * 2.0f << std::endl;
    // apex.x apex.y apex.z apexRadius
    ss << "0 0 " << length * 1.5f << " 0" << std::endl;

    // Create a cyan material for the -X axis.
    // f red green blue Kd Ks Shine transmittance indexOfRefraction
    ss << "f 0 1 1 1 0 0 0 0" << std::endl;

    // Create a cylinder aligned to the -X axis.
    ss << "c" << std::endl;
    // base.x base.y base.z baseRadius
    ss << "0 0 0 " << radius << std::endl;
    // apex.x apex.y apex.z apexRadius
    ss << -length << " 0 0 " << radius << std::endl;

    // Create a cone for the -X axis.
    ss << "c" << std::endl;
    // base.x base.y base.z baseRadius
    ss << -length << " 0 0 " << radius * 2.0f << std::endl;
    // apex.x apex.y apex.z apexRadius
    ss << -length * 1.5f << " 0 0 0" << std::endl;

    // Create a yellow material for the -Y axis.
    // f red green blue Kd Ks Shine transmittance indexOfRefraction
    ss << "f 1 0 1 1 0 0 0 0" << std::endl;

    // Create a cylinder aligned to the -Y axis.
    ss << "c" << std::endl;
    // base.x base.y base.z baseRadius
    ss << "0 0 0 " << radius << std::endl;
    // apex.x apex.y apex.z apexRadius
    ss << "0 " << -length << " 0 " << radius << std::endl;

    // Create a cone for the -Y axis.
    ss << "c" << std::endl;
    // base.x base.y base.z baseRadius
    ss << "0 " << -length << " 0 0" << std::endl;
    // apex.x apex.y apex.z apexRadius
    ss << "0 " << -length * 1.5f << " 0 " << radius * 2.0f << std::endl;

    // Create a magenta material for the -Z axis.
    // f red green blue Kd Ks Shine transmittance indexOfRefraction
    ss << "f 1 1 0 1 0 0 0 0" << std::endl;

    // Create a cylinder aligned to the -Z axis.
    ss << "c" << std::endl;
    // base.x base.y base.z baseRadius
    ss << "0 0 0 " << radius << std::endl;
    // apex.x apex.y apex.z apexRadius
    ss << "0 0 " << -length << " " << radius << std::endl;

    // Create a cone for the -Z axis.
    ss << "c" << std::endl;
    // base.x base.y base.z baseRadius
    ss << "0 0 " << -length << " " << radius * 2.0f << std::endl;
    // apex.x apex.y apex.z apexRadius
    ss << "0 0 " << -length * 1.5f << " 0" << std::endl;

    if ( scene->LoadFromString( ss.str(), "nff" ) )
    {
        return scene;
    }

    // An error occurred while loading the scene.
    DestroyScene( scene );
    return nullptr;

}

void RenderDeviceDX11::DestroyScene( std::shared_ptr<Scene> scene )
{
    SceneList::iterator iter = std::find( m_Scenes.begin(), m_Scenes.end(), scene );
    if ( iter != m_Scenes.end() )
    {
        m_Scenes.erase( iter );
    }
}

std::shared_ptr<Shader> RenderDeviceDX11::CreateShader()
{
    std::shared_ptr<Shader> pShader = std::make_shared<ShaderDX11>( m_pDevice.Get() );
    m_Shaders.push_back( pShader );

    return pShader;
}

void RenderDeviceDX11::DestroyShader( std::shared_ptr<Shader> shader )
{
    ShaderList::iterator iter = std::find( m_Shaders.begin(), m_Shaders.end(), shader );
    if ( iter != m_Shaders.end() )
    {
        m_Shaders.erase( iter );
    }
}

std::shared_ptr<Texture> RenderDeviceDX11::CreateTexture( const std::wstring& fileName )
{
    TextureMap::iterator iter = m_TexturesByName.find( fileName );
    if ( iter != m_TexturesByName.end() )
    {
        return iter->second;
    }

    std::shared_ptr<Texture> texture = std::make_shared<TextureDX11>( m_pDevice.Get() );
    texture->LoadTexture2D( fileName );

    m_Textures.push_back( texture );
    m_TexturesByName.insert( TextureMap::value_type(fileName, texture ) );

    return texture;
}

std::shared_ptr<Texture> RenderDeviceDX11::CreateTextureCube( const std::wstring& fileName )
{
    TextureMap::iterator iter = m_TexturesByName.find( fileName );
    if ( iter != m_TexturesByName.end() )
    {
        return iter->second;
    }

    std::shared_ptr<Texture> texture = std::make_shared<TextureDX11>( m_pDevice.Get() );
    texture->LoadTextureCube( fileName );

    m_Textures.push_back( texture );
    m_TexturesByName.insert( TextureMap::value_type( fileName, texture ) );

    return texture;

}


std::shared_ptr<Texture> RenderDeviceDX11::CreateTexture1D( uint16_t width, uint16_t slices, const Texture::TextureFormat& format, CPUAccess cpuAccess, bool gpuWrite )
{
    std::shared_ptr<Texture> texture = std::make_shared<TextureDX11>( m_pDevice.Get(), width, slices, format, cpuAccess, gpuWrite );
    m_Textures.push_back( texture );

    return texture;
}

std::shared_ptr<Texture> RenderDeviceDX11::CreateTexture2D( uint16_t width, uint16_t height, uint16_t slices, const Texture::TextureFormat& format, CPUAccess cpuAccess, bool gpuWrite )
{
    std::shared_ptr<Texture> texture = std::make_shared<TextureDX11>( m_pDevice.Get(), width, height, slices, format, cpuAccess, gpuWrite );
    m_Textures.push_back( texture );

    return texture;
}

std::shared_ptr<Texture> RenderDeviceDX11::CreateTexture3D( uint16_t width, uint16_t height, uint16_t depth, const Texture::TextureFormat& format, CPUAccess cpuAccess, bool gpuWrite )
{
    std::shared_ptr<Texture> texture = std::make_shared<TextureDX11>( TextureDX11::Tex3d, m_pDevice.Get(), width, height, depth, format, cpuAccess, gpuWrite );
    m_Textures.push_back( texture );

    return texture;
}

std::shared_ptr<Texture> RenderDeviceDX11::CreateTextureCube( uint16_t size, uint16_t numCubes, const Texture::TextureFormat& format, CPUAccess cpuAccess, bool gpuWrite )
{
    std::shared_ptr<Texture> texture = std::make_shared<TextureDX11>( TextureDX11::Cube, m_pDevice.Get(), size, numCubes, format, cpuAccess, gpuWrite );
    m_Textures.push_back( texture );

    return texture;
}

std::shared_ptr<Texture> RenderDeviceDX11::CreateTexture()
{
    std::shared_ptr<Texture> texture = std::make_shared<TextureDX11>( m_pDevice.Get() );
    m_Textures.push_back( texture );
    
    return texture;
}

std::shared_ptr<Texture> RenderDeviceDX11::GetDefaultTexture() const
{
    return m_pDefaultTexture;
}

void RenderDeviceDX11::DestroyTexture( std::shared_ptr<Texture> texture )
{
    TextureList::iterator iter = std::find( m_Textures.begin(), m_Textures.end(), texture );
    if ( iter != m_Textures.end() )
    {
        m_Textures.erase( iter );
    }

    TextureMap::iterator iter2 = std::find_if( m_TexturesByName.begin(), m_TexturesByName.end(), [=] ( TextureMap::value_type val ) { return ( val.second == texture ); } );
    if ( iter2 != m_TexturesByName.end() )
    {
        m_TexturesByName.erase( iter2 );
    }
}

std::shared_ptr<RenderTarget> RenderDeviceDX11::CreateRenderTarget()
{
    std::shared_ptr<RenderTargetDX11> renderTarget = std::make_shared<RenderTargetDX11>( m_pDevice.Get() );
    m_RenderTargets.push_back( renderTarget );

    return renderTarget;
}

void RenderDeviceDX11::DestroyRenderTarget( std::shared_ptr<RenderTarget> renderTarget )
{
    RenderTargetList::iterator iter = std::find( m_RenderTargets.begin(), m_RenderTargets.end(), renderTarget );
    if ( iter != m_RenderTargets.end() )
    {
        m_RenderTargets.erase( iter );
    }
}

std::shared_ptr<SamplerState> RenderDeviceDX11::CreateSamplerState()
{
    std::shared_ptr<SamplerState> sampler = std::make_shared<SamplerStateDX11>( m_pDevice.Get() );
    m_Samplers.push_back( sampler );

    return sampler;
}

void RenderDeviceDX11::DestroySampler( std::shared_ptr<SamplerState> sampler )
{
    SamplerList::iterator iter = std::find( m_Samplers.begin(), m_Samplers.end(), sampler );
    if ( iter != m_Samplers.end() )
    {
        m_Samplers.erase( iter );
    }
}

std::shared_ptr<Material> RenderDeviceDX11::CreateMaterial()
{
    std::shared_ptr<Material> pMaterial = std::make_shared<Material>( *this );
    m_Materials.push_back( pMaterial );
    return pMaterial;
}

void RenderDeviceDX11::DestroyMaterial( std::shared_ptr<Material> material )
{
    MaterialList::iterator iter = std::find( m_Materials.begin(), m_Materials.end(), material );
    if ( iter != m_Materials.end() )
    {
        m_Materials.erase( iter );
    }
}

std::shared_ptr<PipelineState> RenderDeviceDX11::CreatePipelineState()
{
    std::shared_ptr<PipelineState> pPipeline = std::make_shared<PipelineStateDX11>( m_pDevice.Get() );
    m_Pipelines.push_back( pPipeline );
    
    return pPipeline;
}

void RenderDeviceDX11::DestoryPipelineState( std::shared_ptr<PipelineState> pipeline )
{
    PipelineList::iterator iter = std::find( m_Pipelines.begin(), m_Pipelines.end(), pipeline );
    if ( iter != m_Pipelines.end() )
    {
        m_Pipelines.erase( iter );
    }
}

std::shared_ptr<Query> RenderDeviceDX11::CreateQuery( Query::QueryType queryType, uint8_t numBuffers )
{
    std::shared_ptr<Query> query = std::make_shared<QueryDX11>( m_pDevice.Get(), queryType, numBuffers );
    m_Queries.push_back( query );

    return query;
}

void RenderDeviceDX11::DestoryQuery( std::shared_ptr<Query> query )
{
    QueryList::iterator iter = std::find( m_Queries.begin(), m_Queries.end(), query );
    if ( iter != m_Queries.end() )
    {
        m_Queries.erase( iter );
    }
}

void RenderDeviceDX11::OnInitialize( EventArgs& e )
{
    LoadDefaultResources();
}

void RenderDeviceDX11::OnLoadingProgress( ProgressEventArgs& e )
{
    base::OnLoadingProgress( e );
}

void RenderDeviceDX11::LoadDefaultResources()
{
    // Load a default shader
    std::string defaultShaderSource = GetStringResource( DEFAULT_SHADER, "Shader" );

    std::shared_ptr<Shader> pDefaultVertexShader = CreateShader();
    pDefaultVertexShader->LoadShaderFromString( Shader::VertexShader, defaultShaderSource, L"DefaultShader.hlsl", Shader::ShaderMacros(), "VS_main", "vs_4_0" );

    std::shared_ptr<Shader> pDefaultPixelShader = CreateShader();
    pDefaultPixelShader->LoadShaderFromString( Shader::PixelShader, defaultShaderSource, L"DefaultShader.hlsl", Shader::ShaderMacros(), "PS_main", "ps_4_0" );

    // Create a magenta texture if a texture defined in the shader is not bound.
    m_pDefaultTexture = CreateTexture2D( 1, 1, 1, Texture::TextureFormat() );
    m_pDefaultTexture->Clear( ClearFlags::Color, glm::vec4( 1, 0, 1, 1 ) );

    m_pDefaultPipeline = CreatePipelineState();

    m_pDefaultPipeline->SetShader( Shader::VertexShader, pDefaultVertexShader );
    m_pDefaultPipeline->SetShader( Shader::PixelShader, pDefaultPixelShader );
    // TODO: Default pipeline state must be assigned to a renderwindow
    // because the RenderWindow has a default render target that must be bound to the pipeline.

}