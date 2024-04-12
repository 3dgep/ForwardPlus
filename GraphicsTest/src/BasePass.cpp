#include <GraphicsTestPCH.h>

#include <Application.h>
#include <RenderDevice.h>
#include <Scene.h>
#include <SceneNode.h>
#include <Mesh.h>
#include <Material.h>
#include <PipelineState.h>
#include <Shader.h>
#include <ShaderParameter.h>
#include <Camera.h>
#include <ConstantBuffer.h>

#include <BasePass.h>

BasePass::BasePass()
    : m_pRenderEventArgs( nullptr )
    , m_RenderDevice( Application::Get().GetRenderDevice() )
{
    m_PerObjectData = (PerObject*)_aligned_malloc( sizeof( PerObject ), 16 );
    m_PerObjectConstantBuffer = m_RenderDevice.CreateConstantBuffer( PerObject() );
}

BasePass::BasePass( std::shared_ptr<Scene> scene, std::shared_ptr<PipelineState> pipeline )
    : m_pRenderEventArgs( nullptr )
    , m_Scene( scene )
    , m_Pipeline( pipeline )
    , m_RenderDevice( Application::Get().GetRenderDevice() )
{
    m_PerObjectData = (PerObject*)_aligned_malloc( sizeof( PerObject ), 16 );
    m_PerObjectConstantBuffer = m_RenderDevice.CreateConstantBuffer( PerObject() );
}

BasePass::~BasePass()
{
    _aligned_free( m_PerObjectData );
    m_RenderDevice.DestroyConstantBuffer( m_PerObjectConstantBuffer );
}

void BasePass::SetPerObjectConstantBufferData( PerObject& perObjectData )
{
    m_PerObjectConstantBuffer->Set( perObjectData );
}

void BasePass::BindPerObjectConstantBuffer( std::shared_ptr<Shader> shader )
{
    if ( shader )
    {
        shader->GetShaderParameterByName( "PerObject" ).Set( m_PerObjectConstantBuffer );
    }
}

void BasePass::PreRender( RenderEventArgs& e )
{
    e.PipelineState = m_Pipeline.get();
    SetRenderEventArgs( e );

    if ( m_Pipeline )
    {
        // Make sure the per object constant buffer is bound to the vertex shader.
        BindPerObjectConstantBuffer( m_Pipeline->GetShader( Shader::VertexShader ) );
        m_Pipeline->Bind();
    }
}

void BasePass::Render( RenderEventArgs& e )
{
    if ( m_Scene )
    {
        m_Scene->Accept( *this );
    }
}

void BasePass::PostRender( RenderEventArgs& e )
{
    if ( m_Pipeline )
    {
        m_Pipeline->UnBind();
    }
}

// Inherited from Visitor
void BasePass::Visit( Scene& scene )
{

}

void BasePass::Visit( SceneNode& node )
{
    Camera* camera = GetRenderEventArgs().Camera;
    if ( camera )
    {
        PerObject perObjectData;
        // Update the constant buffer data for the node.
        glm::mat4 viewMatrix = camera->GetViewMatrix();
        perObjectData.ModelView = viewMatrix * node.GetWorldTransfom();
        perObjectData.ModelViewProjection = camera->GetProjectionMatrix() * perObjectData.ModelView;

        // Update the constant buffer data
        SetPerObjectConstantBufferData( perObjectData );
    }
}

void BasePass::Visit( Mesh& mesh )
{
    std::shared_ptr<Material> pMaterial = mesh.GetMaterial();
    if ( pMaterial && m_pRenderEventArgs )
    {
        mesh.Render( *m_pRenderEventArgs );
    }
}

void BasePass::SetRenderEventArgs( RenderEventArgs& e )
{
    m_pRenderEventArgs = &e;
}

RenderEventArgs& BasePass::GetRenderEventArgs() const
{
    assert( m_pRenderEventArgs );
    return *m_pRenderEventArgs;
}

RenderDevice& BasePass::GetRenderDevice() const
{
    return m_RenderDevice;
}
