#include <GraphicsTestPCH.h>

#include <Application.h>
#include <RenderDevice.h>
#include <PipelineState.h>
#include <Events.h>
#include <Scene.h>
#include <SceneNode.h>
#include <Camera.h>
#include <Material.h>
#include <Mesh.h>

#include <LightsPass.h>

LightsPass::LightsPass( std::vector<Light>& lights, std::shared_ptr<Scene> pointLight, std::shared_ptr<Scene> spotLight, std::shared_ptr<Scene> directionalLight, std::shared_ptr<PipelineState> pipeline )
    : base( std::shared_ptr<Scene>(), pipeline )
    , m_Lights( lights )
    , m_pCurrentLight( nullptr )
    , m_uiLightIndex( (uint32_t)-1 )
    , m_RenderDevice( Application::Get().GetRenderDevice() )
    , m_Pipeline( pipeline )
    , m_PointLightScene( pointLight )
    , m_pSpotLightScene( spotLight )
    , m_pDirectionalLightScene( directionalLight )
{
    m_LightMaterial = m_RenderDevice.CreateMaterial();
}

LightsPass::~LightsPass()
{

}

//void LightsPass::PreRender( RenderEventArgs& e )
//{
//    e.PipelineState = m_Pipeline.get();
//    SetRenderEventArgs( e );
//
//    if ( m_Pipeline )
//    {
//        // Make sure the per object constant buffer is bound to the vertex shader.
//        BindPerObjectConstantBuffer( m_Pipeline->GetShader( Shader::VertexShader ) );
//        m_Pipeline->Bind();
//    }
//}

// Render the pass. This should only be called by the RenderTechnique.
void LightsPass::Render( RenderEventArgs& e )
{
    m_uiLightIndex = 0;

    for ( Light& light : m_Lights )
    {
        m_pCurrentLight = &light;

        // Disabled lights should appear dimmer than enabled ones.
        float alpha = light.m_Enabled ? 0.5f : 0.1f;
        // Selected lights should appear more opaque.
        alpha = light.m_Selected ? 0.9f : alpha;

        m_LightMaterial->SetDiffuseColor( light.m_Color );
        m_LightMaterial->SetOpacity( alpha );

        switch ( light.m_Type )
        {
        case Light::LightType::Point:
            // Render point lights as spheres.
            m_PointLightScene->Accept( *this );
            break;
        case Light::LightType::Spot:
            // Render spot lights as cones.
            m_pSpotLightScene->Accept( *this );
            break;
        case Light::LightType::Directional:
            // Render directional lights as arrows.
            m_pDirectionalLightScene->Accept( *this );
            break;
        }

        ++m_uiLightIndex;
    }
}

// Inherited from Visitor
void LightsPass::Visit( Scene& scene )
{

}

void LightsPass::Visit( SceneNode& node )
{
    Camera* camera = GetRenderEventArgs().Camera;

    PerObject perObjectData;

    glm::mat4 nodeTransform = node.GetWorldTransfom();

    // Setup constant buffer for node.
    // Create a model matrix from the light properties.
    glm::mat4 translation = glm::translate( glm::vec3( m_pCurrentLight->m_PositionWS ) );
    // Create a rotation matrix that rotates the model towards the direction of the light.
    glm::mat4 rotation = glm::toMat4( glm::quat( glm::vec3( 0, 0, 1 ), glm::normalize( glm::vec3( m_pCurrentLight->m_DirectionWS ) ) ) );

    // Compute the scale depending on the light type.
    float scaleX, scaleY, scaleZ;
    // For directional lights, we don't want any scaling applied.
    // For point lights, we want to scale the geometry by the range of the light.
    scaleX = scaleY = scaleZ = ( m_pCurrentLight->m_Type == Light::LightType::Directional ) ? 1.0f : m_pCurrentLight->m_Range;
    if ( m_pCurrentLight->m_Type == Light::LightType::Spot )
    {
        // For spotlights, we want to scale the base of the cone by the spotlight angle.
        scaleX = scaleY = glm::tan( glm::radians( m_pCurrentLight->m_SpotlightAngle ) ) * m_pCurrentLight->m_Range;
    }

    glm::mat4 scale = glm::scale( glm::vec3( scaleX, scaleY, scaleZ ) );

    perObjectData.ModelView = camera->GetViewMatrix() * translation * rotation * scale * nodeTransform;
    perObjectData.ModelViewProjection = camera->GetProjectionMatrix() * perObjectData.ModelView;

    SetPerObjectConstantBufferData( perObjectData );

}

void LightsPass::Visit( Mesh& mesh )
{
    std::shared_ptr<Material> tempMaterial = mesh.GetMaterial();

    // Temporarily replace the material of the mesh
    // for rendering the mesh as a light object.
    mesh.SetMaterial( m_LightMaterial );

    mesh.Render( GetRenderEventArgs() );

    // Restore the mesh's original material.
    mesh.SetMaterial( tempMaterial );
}


const Light* LightsPass::GetCurrentLight()
{
    return m_pCurrentLight;
}

const uint32_t LightsPass::GetCurrentLightIndex()
{
    return m_uiLightIndex;
}
