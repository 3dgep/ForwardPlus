#pragma once

#include "RenderPass.h"

// Most passes only need to override a single method of a RenderPass.
// Passes can derive from this class to minimize the number of methods that 
// must be overriden in order to implement a render pass.
// Most of the methods of the abstract pass resolve to do-nothing functions.
class AbstractPass : public RenderPass
{
public:
    AbstractPass();
    virtual ~AbstractPass();

    // Enable or disable the pass. If a pass is disabled, the technique will skip it.
    virtual void SetEnabled( bool enabled );
    virtual bool IsEnabled() const;

    // Render the pass. This should only be called by the RenderTechnique.
    virtual void PreRender( RenderEventArgs& e );
    virtual void Render( RenderEventArgs& e ) = 0;
    virtual void PostRender( RenderEventArgs& e );

    // Inherited from Visitor
    virtual void Visit( Scene& scene );
    virtual void Visit( SceneNode& node );
    virtual void Visit( Mesh& mesh );

private:
    bool m_Enabled;
};