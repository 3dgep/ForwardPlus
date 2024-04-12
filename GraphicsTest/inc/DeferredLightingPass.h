#pragma once

#pragma once

#include <Light.h>

#include "BasePass.h"

// Use this pass to render the lights as geometry in the scene.
class DeferredLightingPass : public BasePass
{
public:
    typedef BasePass base;

    DeferredLightingPass( std::vector<Light>& lights,
                          std::shared_ptr<Scene> pointLight,
                          std::shared_ptr<Scene> spotLight,
                          std::shared_ptr<PipelineState> lightPipeline0,
                          std::shared_ptr<PipelineState> lightPipeline1,
                          std::shared_ptr<PipelineState> directionalLightPipeline,
                          std::shared_ptr<Texture> diffuseTexture,
                          std::shared_ptr<Texture> specularTexture,
                          std::shared_ptr<Texture> normalTexture,
                          std::shared_ptr<Texture> depthTexture
                        );

    virtual ~DeferredLightingPass();

    // Render the pass. This should only be called by the RenderTechnique.
    virtual void PreRender( RenderEventArgs& e );
    virtual void Render( RenderEventArgs& e );
    virtual void PostRender( RenderEventArgs& e );

    // Inherited from Visitor
    virtual void Visit( Scene& scene );
    virtual void Visit( SceneNode& node );

protected:

    // Render a subpass of the this pass using a specific pipeline.
    void RenderSubPass( RenderEventArgs& e, std::shared_ptr<Scene> scene, std::shared_ptr<PipelineState> pipeline );

private:
    std::vector<Light>& m_Lights;
    // The light we are currently rendering.
    Light* m_pCurrentLight;

    RenderDevice& m_RenderDevice;

    __declspec( align( 16 ) ) struct ScreenToViewParams
    {
        glm::mat4x4 m_InverseProjectionMatrix;
        glm::vec2 m_ScreenDimensions;
    };
    ScreenToViewParams* m_pScreenToViewParams;
    std::shared_ptr<ConstantBuffer> m_ScreenToViewParamsCB;

    __declspec( align( 16 ) ) struct LightParams
    {
        uint32_t m_LightIndex;
    };
    LightParams* m_pLightParams;
    std::shared_ptr<ConstantBuffer> m_LightParamsCB;

    // First pipeline to mark lit pixels.
    std::shared_ptr<PipelineState> m_LightPipeline0;
    // Second pipeline to render lit pixels.
    std::shared_ptr<PipelineState> m_LightPipeline1;
    // Pipeline for directional lights
    std::shared_ptr<PipelineState> m_DirectionalLightPipeline;

    std::shared_ptr<Scene> m_pPointLightScene;
    std::shared_ptr<Scene> m_pSpotLightScene;
    std::shared_ptr<Scene> m_pDirectionalLightScene;

    // Textures
    std::shared_ptr<Texture> m_DiffuseTexture;
    std::shared_ptr<Texture> m_SpecularTexture;
    std::shared_ptr<Texture> m_NormalTexture;
    std::shared_ptr<Texture> m_DepthTexture;

    __declspec( align( 16 ) ) struct AlignedProperties
    {
        glm::mat4 m_OrthographicProjection; // Requires 16 byte alignment.
    };
    AlignedProperties* m_pAlignedProperties;
};