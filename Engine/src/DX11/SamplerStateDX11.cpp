#include <EnginePCH.h>

#include "SamplerStateDX11.h"

SamplerStateDX11::SamplerStateDX11( ID3D11Device2* pDevice )
    : m_pDevice( pDevice )
    , m_pDeviceContext( nullptr )
    , m_pSamplerState( nullptr )
    , m_MinFilter( MinFilter::MinNearest )
    , m_MagFilter( MagFilter::MagNearest )
    , m_MipFilter( MipFilter::MipNearest )
    , m_WrapModeU( WrapMode::Repeat )
    , m_WrapModeV( WrapMode::Repeat )
    , m_WrapModeW( WrapMode::Repeat )
    , m_CompareMode( CompareMode::None )
    , m_CompareFunc( CompareFunc::Always )
    , m_fLODBias( 0.0f )
    , m_fMinLOD( 0.0f )
    , m_fMaxLOD( D3D11_FLOAT32_MAX )
    , m_bIsAnisotropicFilteringEnabled( false )
    , m_AnisotropicFiltering( 1 )
    , m_bIsDirty( true )
{
    if ( m_pDevice )
    {
        m_pDevice->GetImmediateContext2( &m_pDeviceContext );
    }
}

SamplerStateDX11::~SamplerStateDX11()
{}

void SamplerStateDX11::SetFilter( MinFilter minFilter, MagFilter magFilter, MipFilter mipFilter )
{
    m_MinFilter = minFilter;
    m_MagFilter = magFilter;
    m_MipFilter = mipFilter;
    m_bIsDirty = true;
}

void SamplerStateDX11::GetFilter( MinFilter& minFilter, MagFilter& magFilter, MipFilter& mipFilter ) const
{
    minFilter = m_MinFilter;
    magFilter = m_MagFilter;
    mipFilter = m_MipFilter;
}

void SamplerStateDX11::SetWrapMode( WrapMode u, WrapMode v, WrapMode w )
{
    m_WrapModeU = u;
    m_WrapModeV = v;
    m_WrapModeW = w;
    m_bIsDirty = true;
}

void SamplerStateDX11::GetWrapMode( WrapMode& u, WrapMode& v, WrapMode& w ) const
{
    u = m_WrapModeU;
    v = m_WrapModeV;
    w = m_WrapModeW;
}

void SamplerStateDX11::SetCompareFunction( CompareFunc compareFunc )
{
    m_CompareFunc = compareFunc;
    m_bIsDirty = true;
}

SamplerState::CompareFunc SamplerStateDX11::GetCompareFunc() const
{
    return m_CompareFunc;
}

void SamplerStateDX11::SetLODBias( float lodBias )
{
    m_fLODBias = lodBias;
    m_bIsDirty = true;
}

float SamplerStateDX11::GetLODBias() const
{
    return m_fLODBias;
}

void SamplerStateDX11::SetMinLOD( float minLOD )
{
    m_fMinLOD = minLOD;
    m_bIsDirty = true;
}

float SamplerStateDX11::GetMinLOD() const
{
    return m_fMinLOD;
}

void SamplerStateDX11::SetMaxLOD( float maxLOD )
{
    m_fMaxLOD = maxLOD;
    m_bIsDirty = true;
}

float SamplerStateDX11::GetMaxLOD() const
{
    return m_fMaxLOD;
}

void SamplerStateDX11::SetBorderColor( const glm::vec4& borderColor )
{
    m_BorderColor = borderColor;
    m_bIsDirty = true;
}

glm::vec4 SamplerStateDX11::GetBorderColor() const
{
    return m_BorderColor;
}

void SamplerStateDX11::EnableAnisotropicFiltering( bool enabled )
{
    m_bIsAnisotropicFilteringEnabled = enabled;
    m_bIsDirty = true;
}

bool SamplerStateDX11::IsAnisotropicFilteringEnabled() const
{
    return m_bIsAnisotropicFilteringEnabled;
}


void SamplerStateDX11::SetMaxAnisotropy( uint8_t maxAnisotropy )
{
    m_AnisotropicFiltering = glm::clamp<uint8_t>( maxAnisotropy, 1, 16 );
    m_bIsDirty = true;
}

uint8_t SamplerStateDX11::GetMaxAnisotropy() const
{
    return m_AnisotropicFiltering;
}

D3D11_FILTER SamplerStateDX11::TranslateFilter() const
{
    D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    if ( m_bIsAnisotropicFilteringEnabled )
    {
        filter = ( m_CompareMode == CompareMode::None ) ? D3D11_FILTER_ANISOTROPIC : D3D11_FILTER_COMPARISON_ANISOTROPIC;
        return filter;
    }

    if ( m_MinFilter == MinFilter::MinNearest && m_MagFilter == MagFilter::MagNearest && m_MipFilter == MipFilter::MipNearest )
    {
        filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    }
    else if ( m_MinFilter == MinFilter::MinNearest && m_MagFilter == MagFilter::MagNearest && m_MipFilter == MipFilter::MipLinear )
    {
        filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
    }
    else if ( m_MinFilter == MinFilter::MinNearest && m_MagFilter == MagFilter::MagLinear && m_MipFilter == MipFilter::MipNearest )
    {
        filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
    }
    else if ( m_MinFilter == MinFilter::MinNearest && m_MagFilter == MagFilter::MagLinear && m_MipFilter == MipFilter::MipLinear )
    {
        filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
    }
    else if ( m_MinFilter == MinFilter::MinLinear && m_MagFilter == MagFilter::MagNearest && m_MipFilter == MipFilter::MipNearest )
    {
        filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
    }
    else if ( m_MinFilter == MinFilter::MinLinear && m_MagFilter == MagFilter::MagNearest && m_MipFilter == MipFilter::MipLinear )
    {
        filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
    }
    else if ( m_MinFilter == MinFilter::MinLinear && m_MagFilter == MagFilter::MagLinear && m_MipFilter == MipFilter::MipNearest )
    {
        filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    }
    else if ( m_MinFilter == MinFilter::MinLinear && m_MagFilter == MagFilter::MagLinear && m_MipFilter == MipFilter::MipLinear )
    {
        filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    }
    else
    {
        ReportError( "Invalid texture filter modes." );
    }

    if ( m_CompareMode != CompareMode::None )
    {
        *( reinterpret_cast<int*>( &filter ) ) += static_cast<int>( D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT );
    }

    return filter;
}

D3D11_TEXTURE_ADDRESS_MODE SamplerStateDX11::TranslateWrapMode( WrapMode wrapMode ) const
{
    D3D11_TEXTURE_ADDRESS_MODE addressMode = D3D11_TEXTURE_ADDRESS_WRAP;

    switch ( wrapMode )
    {
    case WrapMode::Repeat:
        addressMode = D3D11_TEXTURE_ADDRESS_WRAP;
        break;
    case WrapMode::Clamp:
        addressMode = D3D11_TEXTURE_ADDRESS_CLAMP;
        break;
    case WrapMode::Mirror:
        addressMode = D3D11_TEXTURE_ADDRESS_MIRROR;
        break;
    case WrapMode::Border:
        addressMode = D3D11_TEXTURE_ADDRESS_BORDER;
        break;
    }

    return addressMode;
}

D3D11_COMPARISON_FUNC SamplerStateDX11::TranslateComparisonFunction( CompareFunc compareFunc ) const
{
    D3D11_COMPARISON_FUNC compareFuncD3D11 = D3D11_COMPARISON_ALWAYS;
    switch ( compareFunc )
    {
    case CompareFunc::Never:
        compareFuncD3D11 = D3D11_COMPARISON_NEVER;
        break;
    case CompareFunc::Less:
        compareFuncD3D11 = D3D11_COMPARISON_LESS;
        break;
    case CompareFunc::Equal:
        compareFuncD3D11 = D3D11_COMPARISON_EQUAL;
        break;
    case CompareFunc::LessEqual:
        compareFuncD3D11 = D3D11_COMPARISON_LESS_EQUAL;
        break;
    case CompareFunc::Greater:
        compareFuncD3D11 = D3D11_COMPARISON_GREATER;
        break;
    case CompareFunc::NotEqual:
        compareFuncD3D11 = D3D11_COMPARISON_NOT_EQUAL;
        break;
    case CompareFunc::GreaterEqual:
        compareFuncD3D11 = D3D11_COMPARISON_GREATER_EQUAL;
        break;
    case CompareFunc::Always:
        compareFuncD3D11 = D3D11_COMPARISON_ALWAYS;
        break;
    }

    return compareFuncD3D11;
}

void SamplerStateDX11::Bind( uint32_t ID, Shader::ShaderType shaderType, ShaderParameter::Type parameterType )
{
    if ( m_bIsDirty || m_pSamplerState == nullptr )
    {
        D3D11_SAMPLER_DESC samplerDesc;
        samplerDesc.Filter = TranslateFilter();
        samplerDesc.AddressU = TranslateWrapMode( m_WrapModeU );
        samplerDesc.AddressV = TranslateWrapMode( m_WrapModeV );
        samplerDesc.AddressW = TranslateWrapMode( m_WrapModeW );
        samplerDesc.MipLODBias = m_fLODBias;
        samplerDesc.MaxAnisotropy = m_AnisotropicFiltering;
        samplerDesc.ComparisonFunc = TranslateComparisonFunction( m_CompareFunc );
        samplerDesc.BorderColor[0] = m_BorderColor.r;
        samplerDesc.BorderColor[1] = m_BorderColor.g;
        samplerDesc.BorderColor[2] = m_BorderColor.b;
        samplerDesc.BorderColor[3] = m_BorderColor.a;
        samplerDesc.MinLOD = m_fMinLOD;
        samplerDesc.MaxLOD = m_fMaxLOD;

        m_pDevice->CreateSamplerState( &samplerDesc, &m_pSamplerState );

        m_bIsDirty = false;
    }

    ID3D11SamplerState* pSamplers[] = { m_pSamplerState.Get() };

    switch ( shaderType )
    {
    case Shader::VertexShader:
        m_pDeviceContext->VSSetSamplers( ID, 1, pSamplers );
        break;
    case Shader::TessellationControlShader:
        m_pDeviceContext->HSSetSamplers( ID, 1, pSamplers );
        break;
    case Shader::TessellationEvaluationShader:
        m_pDeviceContext->DSSetSamplers( ID, 1, pSamplers );
        break;
    case Shader::GeometryShader:
        m_pDeviceContext->GSSetSamplers( ID, 1, pSamplers );
        break;
    case Shader::PixelShader:
        m_pDeviceContext->PSSetSamplers( ID, 1, pSamplers );
        break;
    case Shader::ComputeShader:
        m_pDeviceContext->CSSetSamplers( ID, 1, pSamplers );
        break;
    }
}

void SamplerStateDX11::UnBind( uint32_t ID, Shader::ShaderType shaderType, ShaderParameter::Type parameterType )
{
    ID3D11SamplerState* pSamplers[] = { nullptr };

    switch ( shaderType )
    {
    case Shader::VertexShader:
        m_pDeviceContext->VSSetSamplers( ID, 1, pSamplers );
        break;
    case Shader::TessellationControlShader:
        m_pDeviceContext->HSSetSamplers( ID, 1, pSamplers );
        break;
    case Shader::TessellationEvaluationShader:
        m_pDeviceContext->DSSetSamplers( ID, 1, pSamplers );
        break;
    case Shader::GeometryShader:
        m_pDeviceContext->GSSetSamplers( ID, 1, pSamplers );
        break;
    case Shader::PixelShader:
        m_pDeviceContext->PSSetSamplers( ID, 1, pSamplers );
        break;
    case Shader::ComputeShader:
        m_pDeviceContext->CSSetSamplers( ID, 1, pSamplers );
        break;
    }
}

