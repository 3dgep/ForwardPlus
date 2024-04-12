#pragma once

#include <Visitor.h>

class RenderEventArgs;
class Scene;
class SceneNode;
class Mesh;

// A render pass describes a single pass to render a scene.
// This could include opaque pass, transparent pass,
// g-buffer pass, or post process effects that should be applied 
// to a scene before presenting the scene to the final back buffer.
// Passes can be added to a RenderTechnique and the render technique's 
// "Render" method should be used to render all the passes in the order that 
// they are added to the technique.
class RenderPass : public Visitor
{
public:
    // Enable or disable the pass. If a pass is disabled, the technique will skip it.
    virtual void SetEnabled( bool enabled ) = 0;
    virtual bool IsEnabled() const = 0;

    // Render the pass. This should only be called by the RenderTechnique.
    virtual void PreRender( RenderEventArgs& e ) = 0;
    virtual void Render( RenderEventArgs& e ) = 0;
    virtual void PostRender( RenderEventArgs& e ) = 0;

    // Inherited from Visitor
    virtual void Visit( Scene& scene ) = 0;
    virtual void Visit( SceneNode& node ) = 0;
    virtual void Visit( Mesh& mesh ) = 0;
};