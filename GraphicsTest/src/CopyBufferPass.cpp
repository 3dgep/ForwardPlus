#include <GraphicsTestPCH.h>

#include <Buffer.h>
#include <CopyBufferPass.h>

CopyBufferPass::CopyBufferPass( std::shared_ptr<Buffer> destinationBuffer, std::shared_ptr<Buffer> sourceBuffer )
    : m_DestinationBuffer( destinationBuffer )
    , m_SourceBuffer( sourceBuffer )
{}

CopyBufferPass::~CopyBufferPass()
{}

void CopyBufferPass::Render( RenderEventArgs& e )
{
    if ( m_DestinationBuffer )
    {
        m_DestinationBuffer->Copy( m_SourceBuffer );
    }
}
