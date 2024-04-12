#include <GraphicsTestPCH.h>

#include <Application.h>
#include <RenderDevice.h>
#include <ConstantBuffer.h>
#include <PipelineState.h>
#include <ShaderParameter.h>
#include <Events.h>
#include <Scene.h>
#include <SceneNode.h>
#include <Camera.h>
#include <Material.h>
#include <Mesh.h>

#include <DeferredLightingPass.h>

DeferredLightingPass::DeferredLightingPass( std::vector<Light>& lights, 
                                            std::shared_ptr<Scene> pointLight,  
                                            std::shared_ptr<Scene> spotLight, 
                                            std::shared_ptr<PipelineState> lightPipeline0,
                                            std::shared_ptr<PipelineState> lightPipeline1,
                                            std::shared_ptr<PipelineState> directionalLightPipeline,
                                            std::shared_ptr<Texture> diffuseTexture,
                                            std::shared_ptr<Texture> specularTexture,
                                            std::shared_ptr<Texture> normalTexture,
                                            std::shared_ptr<Texture> depthTexture
                                          )
    : m_Lights( lights )
    , m_pCurrentLight( nullptr )
    , m_RenderDevice( Application::Get().GetRenderDevice() )
    , m_LightPipeline0( lightPipeline0 )
    , m_LightPipeline1( lightPipeline1 )
    , m_DirectionalLightPipeline( directionalLightPipeline )
    , m_pPointLightScene( pointLight )
    , m_pSpotLightScene( spotLight )
    , m_DiffuseTexture( diffuseTexture )
    , m_SpecularTexture( specularTexture )
    , m_NormalTexture( normalTexture )
    , m_DepthTexture( depthTexture )
{
    m_pAlignedProperties = (AlignedProperties*)_aligned_malloc( sizeof( AlignedProperties ), 16 );
    m_pScreenToViewParams = (ScreenToViewParams*)_aligned_malloc( sizeof( ScreenToViewParams ), 16 );
    m_pLightParams = (LightParams*)_aligned_malloc( sizeof( LightParams ), 16 );

    m_LightParamsCB = m_RenderDevice.CreateConstantBuffer( LightParams() );
    m_ScreenToViewParamsCB = m_RenderDevice.CreateConstantBuffer( ScreenToViewParams() );

    // Projection matrix for full-screen quad.
    m_pAlignedProperties->m_OrthographicProjection = glm::ortho<float>( -1, 1, -1, 1, 0, 1 );
    // Create a full-screen quad that is placed on the far clip plane.
    m_pDirectionalLightScene = m_RenderDevice.CreateScreenQuad( -1, 1, -1, 1, -1 );
}

DeferredLightingPass::~DeferredLightingPass()
{
    m_RenderDevice.DestroyConstantBuffer( m_LightParamsCB );
    m_RenderDevice.DestroyConstantBuffer( m_ScreenToViewParamsCB );

    _aligned_free( m_pAlignedProperties );
    _aligned_free( m_pScreenToViewParams );
    _aligned_free( m_pLightParams );
}

void DeferredLightingPass::PreRender( RenderEventArgs& e )
{
    // Bind the G-buffer textures to the pixel shader pipeline stage.
    m_DiffuseTexture->Bind( 0, Shader::PixelShader, ShaderParameter::Type::Texture );
    m_SpecularTexture->Bind( 1, Shader::PixelShader, ShaderParameter::Type::Texture );
    m_NormalTexture->Bind( 2, Shader::PixelShader, ShaderParameter::Type::Texture );
    m_DepthTexture->Bind( 3, Shader::PixelShader, ShaderParameter::Type::Texture );
}

void DeferredLightingPass::RenderSubPass( RenderEventArgs& e, std::shared_ptr<Scene> scene, std::shared_ptr<PipelineState> pipeline )
{
    e.PipelineState = pipeline.get();

    // Update the pass's render event args.
    SetRenderEventArgs( e );

    pipeline->Bind();

    scene->Accept( *this );

    pipeline->UnBind();
}

// Render the pass. This should only be called by the RenderTechnique.
void DeferredLightingPass::Render( RenderEventArgs& e )
{
    Camera* pCamera = e.Camera;
    assert( pCamera );

    Viewport viewport = pCamera->GetViewport();
    // We need the inverse projection matrix to compute the view space position of the fragment
    // in the deferred lighting shader.
    m_pScreenToViewParams->m_InverseProjectionMatrix = glm::inverse( pCamera->GetProjectionMatrix() );
    m_pScreenToViewParams->m_ScreenDimensions = glm::vec2( viewport.Width, viewport.Height );

    m_ScreenToViewParamsCB->Set( *m_pScreenToViewParams );

    // Connect shader parameters to shaders.
    // This ensures the right parameters are bound to the rendering pipeline when the shader is bound.
    // (This can probably be done once in the constructor, but if the shaders are recompiled, the parameters
    // need to be reconnected to the shaders.. soooo better safe than sorry I guess).
    std::vector< PipelineState* > pipelines = { m_LightPipeline0.get(), m_LightPipeline1.get(), m_DirectionalLightPipeline.get() };
    for ( auto pipeline : pipelines )
    {
        std::shared_ptr<Shader> vertexShader = pipeline->GetShader( Shader::VertexShader );
        BindPerObjectConstantBuffer( vertexShader );

        std::shared_ptr<Shader> pixelShader = pipeline->GetShader( Shader::PixelShader );
        if ( pixelShader )
        {
            // Bind the per-light & deferred lighting properties constant buffers to the pixel shader.
            pixelShader->GetShaderParameterByName( "LightIndexBuffer" ).Set( m_LightParamsCB );
            pixelShader->GetShaderParameterByName( "ScreenToViewParams" ).Set( m_ScreenToViewParamsCB );
        }
    }

    m_pLightParams->m_LightIndex = 0;
    for ( Light& light : m_Lights )
    {
        if ( light.m_Enabled )
        {
            m_pCurrentLight = &light;

            // Update the constant buffer for the per-light data.
            m_LightParamsCB->Set( *m_pLightParams );

            // Clear the stencil buffer for the next light
            m_LightPipeline0->GetRenderTarget()->Clear( ClearFlags::Stencil, glm::vec4( 0 ), 1.0f, 1 );
            // The other pipelines should have the same render target.. so no need to clear it 3 times.

            switch ( light.m_Type )
            {
            case Light::LightType::Point:
                RenderSubPass( e, m_pPointLightScene, m_LightPipeline0 );
                RenderSubPass( e, m_pPointLightScene, m_LightPipeline1 );
                break;
            case Light::LightType::Spot:
                RenderSubPass( e, m_pSpotLightScene, m_LightPipeline0 );
                RenderSubPass( e, m_pSpotLightScene, m_LightPipeline1 );
                break;
            case Light::LightType::Directional:
                RenderSubPass( e, m_pDirectionalLightScene, m_DirectionalLightPipeline );
                break;
            }
        }
        m_pLightParams->m_LightIndex++;
    }
}

void DeferredLightingPass::PostRender( RenderEventArgs& e )
{
    // Explicitly unbind these textures so they can be used as render target textures.
    m_DiffuseTexture->UnBind( 0, Shader::PixelShader, ShaderParameter::Type::Texture );
    m_SpecularTexture->UnBind( 1, Shader::PixelShader, ShaderParameter::Type::Texture );
    m_NormalTexture->UnBind( 2, Shader::PixelShader, ShaderParameter::Type::Texture );
    m_DepthTexture->UnBind( 3, Shader::PixelShader, ShaderParameter::Type::Texture );
}

// Inherited from Visitor
void DeferredLightingPass::Visit( Scene& scene )
{

}

void DeferredLightingPass::Visit( SceneNode& node )
{
    Camera* camera = GetRenderEventArgs().Camera;

    PerObject perObjectData;

    if ( m_pCurrentLight->m_Type == Light::LightType::Directional )
    {
        perObjectData.ModelView = glm::mat4( 1.0f );
        perObjectData.ModelViewProjection = m_pAlignedProperties->m_OrthographicProjection;
    }
    else
    {
        glm::mat4 nodeTransform = node.GetWorldTransfom();

        // Setup constant buffer for node.
        // Create a model matrix from the light properties.
        glm::mat4 translation = glm::translate( glm::vec3( m_pCurrentLight->m_PositionWS ) );
        // Create a rotation matrix that rotates the model towards the direction of the light.
        glm::mat4 rotation = glm::toMat4( glm::quat( glm::vec3( 0, 0, 1 ), glm::normalize( glm::vec3( m_pCurrentLight->m_DirectionWS ) ) ) );

        // Compute the scale depending on the light type.
        float scaleX, scaleY, scaleZ;
        // For point lights, we want to scale the geometry by the range of the light.
        scaleX = scaleY = scaleZ = m_pCurrentLight->m_Range;
        if ( m_pCurrentLight->m_Type == Light::LightType::Spot )
        {
            // For spotlights, we want to scale the base of the cone by the spotlight angle.
            scaleX = scaleY = glm::tan( glm::radians( m_pCurrentLight->m_SpotlightAngle ) ) * m_pCurrentLight->m_Range;
        }

        glm::mat4 scale = glm::scale( glm::vec3( scaleX, scaleY, scaleZ ) );

        perObjectData.ModelView = camera->GetViewMatrix() * translation * rotation * scale * nodeTransform;
        perObjectData.ModelViewProjection = camera->GetProjectionMatrix() * perObjectData.ModelView;
    }

    SetPerObjectConstantBufferData( perObjectData );
}
