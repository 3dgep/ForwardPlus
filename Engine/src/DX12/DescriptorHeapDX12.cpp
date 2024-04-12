#include <EnginePCH.h>

#include "RenderDeviceDX12.h"

#include "DescriptorHeapDX12.h"

template< typename HandleType >
DescriptorHandleDX12<HandleType>::DescriptorHandleDX12()
    : m_Increment( 0 )
{}

DescriptorHeapDX12::DescriptorHeapDX12( ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count )
    : m_Type( type )
    , m_Size( count )
{
    assert( pDevice && "ID3D12Device must not be NULL");

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = m_Size;
    heapDesc.Type = m_Type;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if ( FAILED( pDevice->CreateDescriptorHeap( &heapDesc, __uuidof( ID3D12DescriptorHeap ), &m_pHeap ) ) )
    {
        ReportError( "Failed to create descriptor heap." );
    }

    m_Increment = pDevice->GetDescriptorHandleIncrementSize( type );
    m_CPUHandle = CPUDescriptorHandleDX12( m_pHeap->GetCPUDescriptorHandleForHeapStart(), m_Increment );
    m_GPUHandle = GPUDescriptorHandleDX12( m_pHeap->GetGPUDescriptorHandleForHeapStart(), m_Increment );
}

CPUDescriptorHandleDX12& DescriptorHeapDX12::GetCPUHandle()
{
    return m_CPUHandle;
}

CPUDescriptorHandleDX12 DescriptorHeapDX12::GetCPUHandleStart() const
{
    return CPUDescriptorHandleDX12( m_pHeap->GetCPUDescriptorHandleForHeapStart(), m_Increment );
}

GPUDescriptorHandleDX12& DescriptorHeapDX12::GetGPUHandle()
{
    return m_GPUHandle;
}

GPUDescriptorHandleDX12 DescriptorHeapDX12::GetGPUHandleStart() const
{
    return GPUDescriptorHandleDX12( m_pHeap->GetGPUDescriptorHandleForHeapStart(), m_Increment );
}

