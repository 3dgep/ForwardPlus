#include <EnginePCH.h>

#include <Events.h>
#include <Material.h>
#include <PipelineState.h>

#include "BufferDX12.h"
// #include "ShaderDX12.h"
#include "MeshDX12.h"

MeshDX12::MeshDX12( ID3D12Device* pDevice )
    : m_pDevice( pDevice )
    , m_pIndexBuffer( NULL )
{}

MeshDX12::~MeshDX12()
{}

void MeshDX12::AddVertexBuffer( const BufferBinding& binding, const Buffer* buffer )
{
    if ( buffer != NULL )
    {
        m_VertexBuffers[binding] = const_cast<Buffer*>( buffer );
    }
}

void MeshDX12::SetIndexBuffer( const Buffer* buffer )
{
    m_pIndexBuffer = const_cast<Buffer*>( buffer );
}

void MeshDX12::SetMaterial( const Material* material )
{
    m_pMaterial = material;
}

void MeshDX12::Render( RenderEventArgs& renderArgs )
{
    assert( m_VertexBuffers.size() > 0 );

    PipelineState* pipeline = renderArgs.PipelineState;
    if ( pipeline )
    {
    //    const ShaderDX12* pVS = dynamic_cast<const ShaderDX12*>( pipeline->GetShader( Shader::VertexShader ) );
    //    const ShaderDX12* pHS = dynamic_cast<const ShaderDX12*>( pipeline->GetShader( Shader::TessellationControlShader ) );
    //    const ShaderDX12* pDS = dynamic_cast<const ShaderDX12*>( pipeline->GetShader( Shader::TessellationEvaluationShader ) );
    //    const ShaderDX12* pGS = dynamic_cast<const ShaderDX12*>( pipeline->GetShader( Shader::GeometryShader ) );
    //    const ShaderDX12* pPS = dynamic_cast<const ShaderDX12*>( pipeline->GetShader( Shader::PixelShader ) );
    //    const ShaderDX12* pCS = dynamic_cast<const ShaderDX12*>( pipeline->GetShader( Shader::ComputeShader ) );

    //    if ( pVS )
    //    {
    //        for ( BufferMap::value_type buffer : m_VertexBuffers )
    //        {
    //            BufferBinding binding = buffer.first;
    //            if ( pVS->HasSemantic( binding ) )
    //            {
    //                UINT slotID = pVS->GetSlotIDBySemantic( binding );
    //                // Bind the vertex buffer to a particular slot ID.
    //                buffer.second->Bind( slotID, Shader::VertexShader );
    //            }
    //        }
    //    }

    //    if ( m_pMaterial )
    //    {
    //        m_pMaterial->Apply( pVS );
    //        m_pMaterial->Apply( pHS );
    //        m_pMaterial->Apply( pDS );
    //        m_pMaterial->Apply( pGS );
    //        m_pMaterial->Apply( pPS );
    //        m_pMaterial->Apply( pCS );
    //    }
    }
    // TODO: The primitive topology should be a parameter.
    // Or specified per index/vertex buffer.
//    m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    if ( m_pIndexBuffer != NULL )
    {
        m_pIndexBuffer->Bind( 0, Shader::VertexShader, ShaderParameter::Type::Buffer );
//        m_pDeviceContext->DrawIndexed( m_pIndexBuffer->GetElementCount(), 0, 0 );
        m_pIndexBuffer->UnBind( 0, Shader::VertexShader, ShaderParameter::Type::Buffer );
    }
    else
    {
        // We assume we have at least one vertex buffer.
        // If not, then why are we rendering this mesh?
        UINT vertexCount = (*m_VertexBuffers.begin()).second->GetElementCount();
//        m_pDeviceContext->Draw( vertexCount, 0 );
    }

    foreach( BufferMap::value_type buffer, m_VertexBuffers )
    {
//        buffer.second->UnBind();
    }
}
