#pragma once

/**
 * A pass to dispatch a compute shader.
 */

#include "AbstractPass.h"

class Shader;

class DispatchPass : public AbstractPass
{
public:
    DispatchPass( std::shared_ptr<Shader> computeShader, const glm::uvec3& numGroups );
    virtual ~DispatchPass();

    // Render the pass. This should only be called by the RenderTechnique.
    virtual void PreRender( RenderEventArgs& e );
    virtual void Render( RenderEventArgs& e );
    virtual void PostRender( RenderEventArgs& e );

    void SetNumGroups( const glm::ivec3& numGroups );
    glm::ivec3 GetNumGroups() const;

private:

    std::shared_ptr<Shader> m_pComputeShader;

    // The number of groups to dispatch for the compute shader kernel.
    glm::uvec3 m_NumGroups;

};