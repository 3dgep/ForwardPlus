#include <GraphicsTestPCH.h>

#include <RenderTarget.h>
#include <Texture.h>

#include <GenerateMipMapsPass.h>

// Generate mipmaps for all of the textures attached to a render target
GenerateMipMapPass::GenerateMipMapPass( std::shared_ptr<RenderTarget> renderTarget )
    : m_RenderTarget( renderTarget )
{

}

// Generate mipmaps for a single texture.
GenerateMipMapPass::GenerateMipMapPass( std::shared_ptr<Texture> texture )
    : m_Texture( texture )
{

}

GenerateMipMapPass::~GenerateMipMapPass()
{}

void GenerateMipMapPass::Render( RenderEventArgs& e )
{
    if ( m_RenderTarget )
    {
        m_RenderTarget->GenerateMipMaps();
    }

    if ( m_Texture )
    {
        m_Texture->GenerateMipMaps();
    }
}
