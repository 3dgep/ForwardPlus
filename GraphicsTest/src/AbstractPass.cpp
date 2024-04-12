#include <GraphicsTestPCH.h>

#include <AbstractPass.h>

AbstractPass::AbstractPass()
    : m_Enabled( true )
{
    int i = 3;
}

AbstractPass::~AbstractPass()
{}

// Enable or disable the pass. If a pass is disabled, the technique will skip it.
void AbstractPass::SetEnabled( bool enabled )
{
    m_Enabled = enabled;
}

bool AbstractPass::IsEnabled() const
{
    return m_Enabled;
}

// Render the pass. This should only be called by the RenderTechnique.
void AbstractPass::PreRender( RenderEventArgs& e )
{}

void AbstractPass::Render( RenderEventArgs& e )
{}

void AbstractPass::PostRender( RenderEventArgs& e )
{}

// Inherited from Visitor
void AbstractPass::Visit( Scene& scene )
{}

void AbstractPass::Visit( SceneNode& node )
{}

void AbstractPass::Visit( Mesh& mesh )
{}
