#include <GraphicsTestPCH.h>

#include <InvokeFunctionPass.h>

InvokeFunctionPass::InvokeFunctionPass( std::function<void( void )> func )
    : m_Func( func )
{}

InvokeFunctionPass::~InvokeFunctionPass()
{

}

void InvokeFunctionPass::Render( RenderEventArgs& e )
{
    if ( m_Func )
    {
        m_Func();
    }
}
