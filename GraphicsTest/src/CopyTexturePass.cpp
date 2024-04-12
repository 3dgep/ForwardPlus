#include <GraphicsTestPCH.h>

#include <Texture.h>

#include <CopyTexturePass.h>

CopyTexturePass::CopyTexturePass( std::shared_ptr<Texture> destinationTexture, std::shared_ptr<Texture> sourceTexture )
    : m_DestinationTexture( destinationTexture )
    , m_SourceTexture( sourceTexture )
{}

CopyTexturePass::~CopyTexturePass()
{}

void CopyTexturePass::Render( RenderEventArgs& e )
{
    if ( m_DestinationTexture )
    {
        m_DestinationTexture->Copy( m_SourceTexture );
    }
}
