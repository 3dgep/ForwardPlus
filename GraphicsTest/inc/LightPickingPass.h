#pragma once

#include "LightsPass.h"

class PipelineState;
struct Light;

/**
 * A rendering pass that is used to 
 * create a texture that can be used to determine
 * which light is clicked with the mouse.
 */
class LightPickingPass : public LightsPass
{
public:
    typedef LightsPass base;

    LightPickingPass( std::vector<Light>& lights, std::shared_ptr<Scene> pointLight, std::shared_ptr<Scene> spotLight, std::shared_ptr<Scene> directionalLight, std::shared_ptr<PipelineState> pipeline );
    virtual ~LightPickingPass();

    virtual void PreRender( RenderEventArgs& e );

    virtual void Visit( Mesh& mesh );

private:

    __declspec( align( 16 ) ) struct LightParams
    {
        uint32_t m_LightIndex;
    };
    LightParams* m_pLightParams;
    std::shared_ptr<ConstantBuffer> m_LightParamsCB;

    RenderDevice& m_RenderDevice;

};