#pragma once

#include <Object.h>
#include <Events.h>

#include "RenderPass.h"

// The rendering technique determines the method used to render a scene.
// Typical techniques include Forward, Deferred shading, or ForwardPlus.
// A rendering technique consists of one or more render passes, for example, 
// a pass for rendering shadow maps, a pass for rendering the opaque geometry of
// the scene, a pass for rendering the transparent geometry, and one or more 
// passes for rendering individual post-process effects.
class RenderTechnique : public Object
{
public:
    RenderTechnique();
    virtual ~RenderTechnique();

    // Add a pass to the technique. The ID of the added pass is returned
    // and can be used to retrieve the pass later.
    virtual unsigned int AddPass( std::shared_ptr<RenderPass> pass );
    virtual std::shared_ptr<RenderPass> GetPass( unsigned int ID ) const;

    // Render the scene using the passes that have been configured.
    virtual void Render( RenderEventArgs& renderEventArgs );

protected:

private:
    typedef std::vector< std::shared_ptr<RenderPass> > RenderPassList;
    RenderPassList m_Passes;
    
};