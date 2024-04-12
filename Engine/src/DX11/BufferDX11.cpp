#include <EnginePCH.h>

#include "BufferDX11.h"

BufferDX11::BufferDX11( ID3D11Device2* pDevice, UINT bindFlags, const void* data, size_t count, UINT stride )
    : m_pDevice( pDevice )
    , m_pDeviceContext( NULL )
    , m_pBuffer( NULL )
    , m_uiStride( stride )
    , m_BindFlags( bindFlags )
    , m_uiCount( (UINT)count )
    , m_bIsBound( false )
{
    D3D11_BUFFER_DESC bufferDesc;
    D3D11_SUBRESOURCE_DATA resourceData;

    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = bindFlags;
    bufferDesc.ByteWidth = m_uiStride * m_uiCount;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    resourceData.pSysMem = data;
    resourceData.SysMemPitch = 0;
    resourceData.SysMemSlicePitch = 0;

    if ( FAILED( m_pDevice->CreateBuffer( &bufferDesc, &resourceData, &m_pBuffer ) ) )
    {
        ReportError( "Failed to create buffer." );
    }

    m_pDevice->GetImmediateContext2( &m_pDeviceContext );
}

BufferDX11::~BufferDX11()
{}

bool BufferDX11::Bind( unsigned int id, Shader::ShaderType shaderType, ShaderParameter::Type parameterType )
{
    assert( m_pDeviceContext );

    ID3D11Buffer* buffers[] = { m_pBuffer.Get() };
    UINT offsets[] = { 0 };
    UINT strides[] = { m_uiStride };

    switch ( m_BindFlags )
    {
    case D3D11_BIND_VERTEX_BUFFER:
        m_pDeviceContext->IASetVertexBuffers( id, 1, buffers, strides, offsets );
        m_bIsBound = true;
        break;
    case D3D11_BIND_INDEX_BUFFER:
        m_pDeviceContext->IASetIndexBuffer( m_pBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );
        m_bIsBound = true;
        break;
    default:
        throw new std::exception( "BufferDX11::Bind: Unimplemented buffer type." );
        // return false;
        break;
    }

    return true;
}

void BufferDX11::UnBind( unsigned int id, Shader::ShaderType shaderType, ShaderParameter::Type parameterType )
{
    ID3D11Buffer* buffers[] = { nullptr };

    switch ( m_BindFlags )
    {
    case D3D11_BIND_VERTEX_BUFFER:
        m_pDeviceContext->IASetVertexBuffers( id, 1, buffers, nullptr, nullptr );
        m_bIsBound = true;
        break;
    case D3D11_BIND_INDEX_BUFFER:
        m_pDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0 );
        m_bIsBound = true;
        break;
    default:
        throw new std::exception( "BufferDX11::Bind: Unimplemented buffer type." );
        // return false;
        break;
    }
}

void BufferDX11::Copy( std::shared_ptr<Buffer> other )
{
    std::shared_ptr<BufferDX11> srcBuffer = std::dynamic_pointer_cast<BufferDX11>( other );

    if ( srcBuffer && srcBuffer.get() != this &&
         m_uiCount * m_uiStride == srcBuffer->m_uiCount * srcBuffer->m_uiStride )
    {
        m_pDeviceContext->CopyResource( m_pBuffer.Get(), srcBuffer->m_pBuffer.Get() );
    }
    else
    {
        ReportError( "Source buffer is not compatible with this buffer." );
    }
}

Buffer::BufferType BufferDX11::GetType() const
{
    switch ( m_BindFlags )
    {
    case D3D11_BIND_VERTEX_BUFFER:
        return Buffer::VertexBuffer;
        break;
    case D3D11_BIND_INDEX_BUFFER:
        return Buffer::IndexBuffer;
        break;
    case D3D11_BIND_CONSTANT_BUFFER:
        return Buffer::ConstantBuffer;
        break;
    default:
        return Buffer::Unknown;
        break;
    }
}

unsigned int BufferDX11::GetElementCount() const
{
    return m_uiCount;
}