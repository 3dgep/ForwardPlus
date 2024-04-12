#pragma once

#include <SamplerState.h>
#include <ShaderParameter.h>

class SamplerStateDX11 : public SamplerState
{
public:
    SamplerStateDX11( ID3D11Device2* pDevice );
    virtual ~SamplerStateDX11();

    virtual void SetFilter( MinFilter minFilter, MagFilter magFilter, MipFilter mipFilter );
    virtual void GetFilter( MinFilter& minFilter, MagFilter& magFilter, MipFilter& mipFilter ) const;

    virtual void SetWrapMode( WrapMode u = WrapMode::Repeat, WrapMode v = WrapMode::Repeat, WrapMode w = WrapMode::Repeat );
    virtual void GetWrapMode( WrapMode& u, WrapMode& v, WrapMode& w ) const;

    virtual void SetCompareFunction( CompareFunc compareFunc );
    virtual CompareFunc GetCompareFunc() const;

    virtual void SetLODBias( float lodBias );
    virtual float GetLODBias() const;

    virtual void SetMinLOD( float minLOD );
    virtual float GetMinLOD() const;

    virtual void SetMaxLOD( float maxLOD );
    virtual float GetMaxLOD() const;

    virtual void SetBorderColor( const glm::vec4& borderColor );
    virtual glm::vec4 GetBorderColor() const;

    virtual void EnableAnisotropicFiltering( bool enabled );
    virtual bool IsAnisotropicFilteringEnabled() const;

    virtual void SetMaxAnisotropy( uint8_t maxAnisotropy );
    virtual uint8_t GetMaxAnisotropy() const;

    virtual void Bind( uint32_t ID, Shader::ShaderType shaderType, ShaderParameter::Type parameterType );
    virtual void UnBind( uint32_t ID, Shader::ShaderType shaderType, ShaderParameter::Type parameterType );

protected:
    
    D3D11_FILTER TranslateFilter() const;
    D3D11_TEXTURE_ADDRESS_MODE TranslateWrapMode( WrapMode wrapMode ) const;
    D3D11_COMPARISON_FUNC TranslateComparisonFunction( CompareFunc compareFunc ) const;

private:
    Microsoft::WRL::ComPtr<ID3D11Device2> m_pDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext2> m_pDeviceContext;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSamplerState;

    MinFilter m_MinFilter;
    MagFilter m_MagFilter;
    MipFilter m_MipFilter;
    WrapMode  m_WrapModeU, m_WrapModeV, m_WrapModeW;
    CompareMode m_CompareMode;
    CompareFunc m_CompareFunc;

    float       m_fLODBias;
    float       m_fMinLOD;
    float       m_fMaxLOD;
    
    glm::vec4   m_BorderColor;

    bool        m_bIsAnisotropicFilteringEnabled;
    uint8_t     m_AnisotropicFiltering;

    // Set to true if the sampler state needs to be recreated.
    bool        m_bIsDirty;
};