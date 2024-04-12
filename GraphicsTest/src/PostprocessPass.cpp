#include <GraphicsTestPCH.h>

#include <Texture.h>

#include <PostprocessPass.h>

PostprocessPass::PostprocessPass( std::shared_ptr<Scene> scene, std::shared_ptr<PipelineState> pipeline, const glm::mat4& projectionMatrix, std::shared_ptr<Texture> texture )
    : base( scene, pipeline )
    , m_ProjectionMatrix( projectionMatrix)
    , m_Texture( texture )
{}

void PostprocessPass::Render( RenderEventArgs& e )
{
    PerObject perObjectData;
    perObjectData.ModelView = glm::mat4( 1.0f ); // Identity
    perObjectData.ModelViewProjection = m_ProjectionMatrix;

    SetPerObjectConstantBufferData( perObjectData );

    if ( m_Texture )
    {
        // Bind the texture to be used as the source for the post process pass to slot 0 of the pixel shader.
        m_Texture->Bind( 0, Shader::PixelShader, ShaderParameter::Type::Texture );
    }

    base::Render( e );

    if ( m_Texture )
    {
        m_Texture->UnBind( 0, Shader::PixelShader, ShaderParameter::Type::Texture );
    }
}

void PostprocessPass::Visit( SceneNode& node )
{
    // Do nothing in this case
}
