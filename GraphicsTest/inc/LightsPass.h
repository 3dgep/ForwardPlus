#pragma once

#include <Light.h>

#include "BasePass.h"

// Use this pass to render the lights as geometry in the scene.
class LightsPass : public BasePass
{
public:
    typedef BasePass base;

    LightsPass( std::vector<Light>& lights, std::shared_ptr<Scene> pointLight, std::shared_ptr<Scene> spotLight, std::shared_ptr<Scene> directionalLight, std::shared_ptr<PipelineState> pipeline );
    virtual ~LightsPass();

    // Render the pass. This should only be called by the RenderTechnique.
//    virtual void PreRender( RenderEventArgs& e );
    virtual void Render( RenderEventArgs& e );

    // Inherited from Visitor
    virtual void Visit( Scene& scene );
    virtual void Visit( SceneNode& node );
    virtual void Visit( Mesh& mesh );

protected:
    const Light* GetCurrentLight();
    const uint32_t GetCurrentLightIndex();

private:
    std::vector<Light>& m_Lights;
    // The light we are currently rendering.
    Light* m_pCurrentLight;
    uint32_t m_uiLightIndex;

    RenderDevice& m_RenderDevice;

    // A material that can be used to render the lights as geometry in the scene.
    std::shared_ptr<Material> m_LightMaterial;

    std::shared_ptr<PipelineState> m_Pipeline;

    std::shared_ptr<Scene> m_PointLightScene;
    std::shared_ptr<Scene> m_pSpotLightScene;
    std::shared_ptr<Scene> m_pDirectionalLightScene;

};