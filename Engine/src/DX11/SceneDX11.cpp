#include <EnginePCH.h>

#include <Texture.h>

#include "RenderDeviceDX11.h"

#include "SceneDX11.h"

SceneDX11::SceneDX11( RenderDeviceDX11& device )
    : m_Device( device )
    , m_pDevice( device.GetDevice() )
    , m_pContext( device.GetDeviceContext() )
{
}

SceneDX11::~SceneDX11()
{}

std::shared_ptr<Buffer> SceneDX11::CreateFloatVertexBuffer( const float* data, unsigned int count, unsigned int stride ) const
{
    return m_Device.CreateFloatVertexBuffer( data, count, stride );
}

std::shared_ptr<Buffer> SceneDX11::CreateUIntIndexBuffer( const unsigned int* data, unsigned int count ) const
{
    return m_Device.CreateUIntIndexBuffer( data, count );
}

std::shared_ptr<Mesh> SceneDX11::CreateMesh() const
{
    return m_Device.CreateMesh();
}

std::shared_ptr<Material> SceneDX11::CreateMaterial() const
{
    return m_Device.CreateMaterial();
}

std::shared_ptr<Texture> SceneDX11::CreateTexture( const std::wstring& fileName ) const
{
    return m_Device.CreateTexture( fileName );
}

std::shared_ptr<Texture> SceneDX11::CreateTexture2D( uint16_t width, uint16_t height )
{
    return m_Device.CreateTexture2D( width, height );
}

std::shared_ptr<Texture> SceneDX11::GetDefaultTexture()
{
    return m_Device.GetDefaultTexture();
}