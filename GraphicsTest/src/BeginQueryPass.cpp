#include <GraphicsTestPCH.h>

#include <Events.h>
#include <Query.h>
#include <BeginQueryPass.h>

BeginQueryPass::BeginQueryPass( std::shared_ptr<Query> query )
    : m_pQuery( query )
{}

BeginQueryPass::~BeginQueryPass()
{}

void BeginQueryPass::Render( RenderEventArgs& e )
{
    if ( m_pQuery )
    {
        m_pQuery->Begin( e.FrameCounter );
    }
}
