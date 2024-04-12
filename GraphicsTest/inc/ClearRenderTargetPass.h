#pragma once

#include "AbstractPass.h"

#include <ClearFlags.h>

class RenderTarget;
class Texture;

/**
 * A render pass that simply clears a render target.
 */
class ClearRenderTargetPass : public AbstractPass
{
public:
    ClearRenderTargetPass( std::shared_ptr<RenderTarget> renderTarget,
                           ClearFlags clearFlags = ClearFlags::All,
                           const glm::vec4& color = glm::vec4( 0 ),
                           float depth = 1.0f,
                           uint8_t stencil = 0 );
    ClearRenderTargetPass( std::shared_ptr<Texture> texture,
                           ClearFlags clearFlags = ClearFlags::All,
                           const glm::vec4& color = glm::vec4( 0 ),
                           float depth = 1.0f,
                           uint8_t stencil = 0 );
    virtual ~ClearRenderTargetPass();

    virtual void Render( RenderEventArgs& e );

private:
    std::shared_ptr<RenderTarget> m_RenderTarget;
    std::shared_ptr<Texture> m_Texture;
    ClearFlags m_ClearFlags;
    glm::vec4 m_ClearColor;
    float m_ClearDepth;
    uint8_t m_ClearStencil;
};
