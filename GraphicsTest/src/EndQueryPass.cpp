#include <GraphicsTestPCH.h>

#include <Events.h>
#include <Query.h>
#include <EndQueryPass.h>

EndQueryPass::EndQueryPass( std::shared_ptr<Query> query )
    : m_pQuery( query )
{}

EndQueryPass::~EndQueryPass()
{}

void EndQueryPass::Render( RenderEventArgs& e )
{
    if ( m_pQuery )
    {
        m_pQuery->End( e.FrameCounter );
    }
}
