#pragma once

#include "Object.h"

#include "Shader.h"
#include "BlendState.h"
#include "RasterizerState.h"
#include "DepthStencilState.h"
#include "RenderTarget.h"

/**
 * The PipelineState class encompasses pipeline configuration.
 * This includes, shaders, blend states, rasterizer states, 
 * and depth/stencil states among other things....
 * 
 * // TODO: RenderTargets...
 */
class PipelineState : public Object
{
public:
    /**
     * An associated map that associates a shader to a type.
     */
    typedef std::map<Shader::ShaderType, std::shared_ptr<Shader> > ShaderMap;

    virtual void SetShader( Shader::ShaderType type, std::shared_ptr<Shader> pShader ) = 0;
    virtual std::shared_ptr<Shader> GetShader( Shader::ShaderType type ) const = 0;
    // Get all shaders associates with this pipeline.
    virtual const ShaderMap& GetShaders() const = 0;


    virtual void SetBlendState( const BlendState& blendState ) = 0;
    virtual BlendState& GetBlendState() = 0;

    virtual void SetRasterizerState( const RasterizerState& rasterizerState ) = 0;
    virtual RasterizerState& GetRasterizerState() = 0;

    virtual void SetDepthStencilState( const DepthStencilState& depthStencilState ) = 0;
    virtual DepthStencilState& GetDepthStencilState() = 0;

    virtual void SetRenderTarget( std::shared_ptr<RenderTarget> renderTarget ) = 0;
    virtual std::shared_ptr<RenderTarget> GetRenderTarget() const = 0;

    // Bind this pipeline state for rendering.
    virtual void Bind() = 0;
    // Unbind render target and shaders.
    virtual void UnBind() = 0;
};