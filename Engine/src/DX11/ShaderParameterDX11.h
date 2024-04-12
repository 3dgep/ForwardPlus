#pragma once

// Inspiration for a shader parameter class:
// http://asawicki.info/news_1529_cconstantbuffer_ctypedconstantbuffer.html
// Accessed: Feb 18, 2013

#include <ShaderParameter.h>
#include <Shader.h>

class ShaderParameterDX11 : public ShaderParameter
{
public:
	typedef ShaderParameter base;

    ShaderParameterDX11();

    // Shader resource parameter.
    ShaderParameterDX11( const std::string& name, UINT slotID, Shader::ShaderType shaderType, Type parameterType );

    bool IsValid() const;

    // Get the type of the stored parameter.
    virtual Type GetType() const;

    // Bind the shader parameter to a specific slot for the given shader type.
    virtual void Bind();
    virtual void UnBind();

protected:

    virtual void SetConstantBuffer( std::shared_ptr<ConstantBuffer> buffer );
    virtual void SetTexture( std::shared_ptr<Texture> texture );
    virtual void SetSampler( std::shared_ptr<SamplerState> sampler );
    virtual void SetStructuredBuffer( std::shared_ptr<StructuredBuffer> rwBuffer );

private:
    std::string m_Name;

    // Shader parameter does not take ownership of these types.
    std::weak_ptr<Texture> m_pTexture;
    std::weak_ptr<SamplerState> m_pSamplerState;
    std::weak_ptr<ConstantBuffer> m_pConstantBuffer;
    std::weak_ptr<StructuredBuffer> m_pStructuredBuffer;

    UINT m_uiSlotID;
    Shader::ShaderType m_ShaderType;
    Type m_ParameterType;
};