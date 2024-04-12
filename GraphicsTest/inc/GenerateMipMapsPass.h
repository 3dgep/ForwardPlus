#pragma once

#include "AbstractPass.h"

class RenderTarget;
class Textre;

/**
* A render pass that simply clears a render target.
*/
class GenerateMipMapPass : public AbstractPass
{
public:
    // Generate mipmaps for all of the textures attached to a render target
    GenerateMipMapPass( std::shared_ptr<RenderTarget> renderTarget );
    // Generate mipmaps for a single texture.
    GenerateMipMapPass( std::shared_ptr<Texture> texture );
    virtual ~GenerateMipMapPass();

    virtual void Render( RenderEventArgs& e );

private:
    std::shared_ptr<RenderTarget> m_RenderTarget;
    std::shared_ptr<Texture> m_Texture;
};
