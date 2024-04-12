#pragma once

#include "AbstractPass.h"

class Texture;

/**
* A render pass that copies one texture to another.
* Texture dimensions and formats of source and destination textures must match.
*/
class CopyTexturePass : public AbstractPass
{
public:
    CopyTexturePass( std::shared_ptr<Texture> destinationTexture,
                     std::shared_ptr<Texture> sourceTexture );
    virtual ~CopyTexturePass();

    virtual void Render( RenderEventArgs& e );

private:
    std::shared_ptr<Texture> m_SourceTexture;
    std::shared_ptr<Texture> m_DestinationTexture;
};
