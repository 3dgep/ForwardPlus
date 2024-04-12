#include <EnginePCH.h>

//#include "ShaderParameterData.h"
#include "ShaderParameterDX11.h"
#include "ConstantBufferDX11.h"
#include "TextureDX11.h"
#include "SamplerStateDX11.h"
#include "StructuredBufferDX11.h"

ShaderParameterDX11::ShaderParameterDX11()
    : m_uiSlotID( UINT_MAX )
    , m_ParameterType( Type::Invalid )
{}

ShaderParameterDX11::ShaderParameterDX11( const std::string& name, UINT slotID, Shader::ShaderType shaderType, Type parameterType )
    : m_Name(name)
    , m_uiSlotID( slotID )
    , m_ShaderType( shaderType )
    , m_ParameterType( parameterType )
{}

void ShaderParameterDX11::SetConstantBuffer( std::shared_ptr<ConstantBuffer> buffer )
{
    m_pConstantBuffer = buffer;
}

void ShaderParameterDX11::SetTexture( std::shared_ptr<Texture> texture )
{
    m_pTexture = texture;
}

void ShaderParameterDX11::SetSampler( std::shared_ptr<SamplerState> sampler )
{
    m_pSamplerState = sampler;
}

void ShaderParameterDX11::SetStructuredBuffer( std::shared_ptr<StructuredBuffer> rwBuffer )
{
    m_pStructuredBuffer = rwBuffer;
}

bool ShaderParameterDX11::IsValid() const
{
    return m_ParameterType != ShaderParameter::Type::Invalid;
}

ShaderParameter::Type ShaderParameterDX11::GetType() const
{
    return m_ParameterType;
}

void ShaderParameterDX11::Bind()
{
    if ( std::shared_ptr<ConstantBuffer> constantBuffer = m_pConstantBuffer.lock() )
    {
        constantBuffer->Bind( m_uiSlotID, m_ShaderType, m_ParameterType );
    }
    if ( std::shared_ptr<Texture> texture = m_pTexture.lock() )
    {
        texture->Bind( m_uiSlotID, m_ShaderType, m_ParameterType );
    }
    if ( std::shared_ptr<SamplerState> samplerState = m_pSamplerState.lock() )
    {
        samplerState->Bind( m_uiSlotID, m_ShaderType, m_ParameterType );
    }
    if ( std::shared_ptr<StructuredBuffer> buffer = m_pStructuredBuffer.lock() )
    {
        buffer->Bind( m_uiSlotID, m_ShaderType, m_ParameterType );
    }
}

void ShaderParameterDX11::UnBind()
{
    if ( std::shared_ptr<ConstantBuffer> constantBuffer = m_pConstantBuffer.lock() )
    {
        constantBuffer->UnBind( m_uiSlotID, m_ShaderType, m_ParameterType );
    }
    if ( std::shared_ptr<Texture> texture = m_pTexture.lock() )
    {
        texture->UnBind( m_uiSlotID, m_ShaderType, m_ParameterType );
    }
    if ( std::shared_ptr<SamplerState> samplerState = m_pSamplerState.lock() )
    {
        samplerState->UnBind( m_uiSlotID, m_ShaderType, m_ParameterType );
    }
    if ( std::shared_ptr<StructuredBuffer> buffer = m_pStructuredBuffer.lock() )
    {
        buffer->UnBind( m_uiSlotID, m_ShaderType, m_ParameterType );
    }
}
