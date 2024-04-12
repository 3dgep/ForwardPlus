#include <EnginePCH.h>

#include <Material.h>
#include <Events.h>
#include <Visitor.h>

#include "BufferDX11.h"
#include "ShaderDX11.h"
#include "PipelineStateDX11.h"

#include "MeshDX11.h"

MeshDX11::MeshDX11( ID3D11Device2* pDevice )
	: m_pDevice( pDevice )
	, m_pIndexBuffer( nullptr )
    , m_pMaterial( nullptr )
    , m_pDeviceContext( nullptr )
{
	m_pDevice->GetImmediateContext2( &m_pDeviceContext );
}

MeshDX11::~MeshDX11()
{}

void MeshDX11::AddVertexBuffer( const BufferBinding& binding, std::shared_ptr<Buffer> buffer )
{
    m_VertexBuffers[binding] = buffer;
}

void MeshDX11::SetIndexBuffer( std::shared_ptr<Buffer> buffer )
{
    m_pIndexBuffer = buffer;
}

void MeshDX11::SetMaterial( std::shared_ptr<Material> material )
{
    m_pMaterial = material;
}

std::shared_ptr<Material> MeshDX11::GetMaterial() const
{
    return m_pMaterial;
}

void MeshDX11::Render( RenderEventArgs& renderArgs )
{
    std::shared_ptr<ShaderDX11> pVS;

    // Clone this mesh's material in case we want to override the 
    // shaders in the mesh's default material.
    //Material material( *m_pMaterial );

    // Use the vertex shader to convert the buffer semantics to slot ID's
    PipelineState* pipeline = renderArgs.PipelineState;
    if ( pipeline )
    {
        pVS = std::dynamic_pointer_cast<ShaderDX11>( pipeline->GetShader( Shader::VertexShader ) );

        if ( pVS )
        {
            for ( BufferMap::value_type buffer : m_VertexBuffers )
            {
                BufferBinding binding = buffer.first;
                if ( pVS->HasSemantic( binding ) )
                {
                    UINT slotID = pVS->GetSlotIDBySemantic( binding );
                    // Bind the vertex buffer to a particular slot ID.
                    buffer.second->Bind( slotID, Shader::VertexShader, ShaderParameter::Type::Buffer );
                }
            }
        }

        if ( m_pMaterial )
        {
            for ( auto shader : pipeline->GetShaders() )
            {
                m_pMaterial->Bind( shader.second );
            }
        }
    }

	// TODO: The primitive topology should be a parameter.
    // Or we have to have index buffers/vertex buffers for each primitive type...
	m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	if ( m_pIndexBuffer != NULL )
	{
        // TOOD: Primitive reset?
		m_pIndexBuffer->Bind( 0, Shader::VertexShader, ShaderParameter::Type::Buffer );
		m_pDeviceContext->DrawIndexed( m_pIndexBuffer->GetElementCount(), 0, 0 );
		m_pIndexBuffer->UnBind( 0, Shader::VertexShader, ShaderParameter::Type::Buffer );
	}
	else
	{
		// We assume we have at least one vertex buffer.
		// If not, then why are we rendering this mesh?
		UINT vertexCount = (*m_VertexBuffers.begin()).second->GetElementCount();
		m_pDeviceContext->Draw( vertexCount, 0 );
	}

    if ( pVS )
    {
        for ( BufferMap::value_type buffer : m_VertexBuffers )
        {
            BufferBinding binding = buffer.first;
            if ( pVS->HasSemantic( binding ) )
            {
                UINT slotID = pVS->GetSlotIDBySemantic( binding );
                // Bind the vertex buffer to a particular slot ID.
                buffer.second->Bind( slotID, Shader::VertexShader, ShaderParameter::Type::Buffer );
            }
        }
    }

}

void MeshDX11::Accept( Visitor& visitor )
{
    visitor.Visit( *this );
}
