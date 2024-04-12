#pragma once

#include "AbstractPass.h"

class RenderDevice;
class Shader;
class ConstantBuffer;
class Camera;
class Scene;
class PipelineState;
class Query;

// Base pass provides implementations for functions used by most passes.
class BasePass : public AbstractPass
{
public:
    typedef AbstractPass base;

    BasePass();
    BasePass( std::shared_ptr<Scene> scene, std::shared_ptr<PipelineState> pipeline );
    virtual ~BasePass();

    // Render the pass. This should only be called by the RenderTechnique.
    virtual void PreRender( RenderEventArgs& e );
    virtual void Render( RenderEventArgs& e );
    virtual void PostRender( RenderEventArgs& e );

    // Inherited from Visitor
    virtual void Visit( Scene& scene );
    virtual void Visit( SceneNode& node );
    virtual void Visit( Mesh& mesh );

protected:
    // PerObject constant buffer data.
    __declspec( align( 16 ) ) struct PerObject
    {
        glm::mat4 ModelViewProjection;
        glm::mat4 ModelView;
    };

    void SetRenderEventArgs( RenderEventArgs& e );
    RenderEventArgs& GetRenderEventArgs() const;

    RenderDevice& GetRenderDevice() const;

    // Set and bind the constant buffer data.
    void SetPerObjectConstantBufferData( PerObject& perObjectData );
    // Bind the constant to the shader.
    void BindPerObjectConstantBuffer( std::shared_ptr<Shader> shader );

private:

    PerObject* m_PerObjectData;
    std::shared_ptr<ConstantBuffer> m_PerObjectConstantBuffer;

    RenderEventArgs* m_pRenderEventArgs;

    // The pipeline state that should be used to render this pass.
    std::shared_ptr<PipelineState> m_Pipeline;

    // The scene to render.
    std::shared_ptr< Scene > m_Scene;

    RenderDevice& m_RenderDevice;
};