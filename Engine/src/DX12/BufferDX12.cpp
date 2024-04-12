#include <EnginePCH.h>

#include "d3dx12.h"
#include "BufferDX12.h"

BufferDX12::BufferDX12( ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, Buffer::BufferType bufferType, const void* data, size_t count, UINT stride )
    : m_pDevice( pDevice )
    , m_uiStride( stride )
    , m_BufferType( bufferType )
    , m_uiCount( (UINT)count )
{
    HRESULT hr;

    m_uiBufferSize = (UINT)( stride * count );

    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer( m_uiBufferSize );

    hr = m_pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT ),
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        __uuidof( ID3D12Resource ), &m_pResource );

    if ( FAILED( hr ) )
    {
        ReportError( "Failed to create buffer resource." );
    }

    // Create an upload buffer that is used to copy the data into the common buffer.
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap;
    hr = m_pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD ),
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        __uuidof( ID3D12Resource ), &uploadHeap );

    if ( FAILED( hr ) )
    {
        ReportError( "Failed to create uplaod heap." );
    }

    // Copy the data to the upload heap then schedule a copy from the upload heap to the 
    // default resource heap.
    D3D12_SUBRESOURCE_DATA subresourceData;
    subresourceData.pData = (void*)data;
    subresourceData.RowPitch = m_uiBufferSize;
    subresourceData.SlicePitch = subresourceData.RowPitch;

    // Transition our default heap resource from "INITIAL" to "COPY_DEST"
    pCommandList->ResourceBarrier( 1, &CD3DX12_RESOURCE_BARRIER::Transition( m_pResource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST ) );
    // Copy the data 
    UpdateSubresources<1>( pCommandList, m_pResource.Get(), uploadHeap.Get(), 0, 0, 1, &subresourceData );
    // Now transition the resource from the "COPY_DST" state to the "GENERIC_READ" for use the graphics pipeline.
    pCommandList->ResourceBarrier( 1, &CD3DX12_RESOURCE_BARRIER::Transition( m_pResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ ) );

    // Create a buffer description depending on the type.
    switch ( m_BufferType )
    {
    case Buffer::IndexBuffer:
        m_IndexBufferView.BufferLocation = m_pResource->GetGPUVirtualAddress();
        m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
        m_IndexBufferView.SizeInBytes = m_uiBufferSize;
        break;
    case Buffer::VertexBuffer:
        m_VertexBufferView.BufferLocation = m_pResource->GetGPUVirtualAddress();
        m_VertexBufferView.SizeInBytes = m_uiBufferSize;
        m_VertexBufferView.StrideInBytes = m_uiStride;
        break;
    default:
        ReportError( "Unknown buffer type." );
        break;
    }
}

BufferDX12::~BufferDX12()
{
    // Nothing to do here since we rely on 
    // the Microsoft::WRL::ComPtr to release our COM objects for us.
}

void BufferDX12::Copy( std::shared_ptr<Buffer> other )
{
    // TODO:
}

bool BufferDX12::Bind( unsigned int id, Shader::ShaderType shaderType, ShaderParameter::Type parameterType )
{
    throw new std::exception( "Invalid method call on BufferDX12" );
    return false;
}

bool BufferDX12::Bind( unsigned int id, ID3D12GraphicsCommandList* pCommandList )
{

    switch ( m_BufferType )
    {
    case Buffer::VertexBuffer:
    {
        pCommandList->IASetVertexBuffers( id, 1, &m_VertexBufferView );
    }
    break;
    case Buffer::IndexBuffer:
    {
        pCommandList->IASetIndexBuffer( &m_IndexBufferView );
    }
    break;
    default:
    {
        throw new std::exception( "BufferDX12::Bind: Unimplemented buffer type." );
        // return false;
    }
    break;
    }

    return true;
}

void BufferDX12::UnBind( unsigned int id, Shader::ShaderType shaderType, ShaderParameter::Type parameterType )
{
    // TODO
}

Buffer::BufferType BufferDX12::GetType() const
{
    return m_BufferType;
}

unsigned int BufferDX12::GetElementCount() const
{
    return m_uiCount;
}
