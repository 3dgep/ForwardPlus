#include <EnginePCH.h>

#include "RasterizerStateDX11.h"

RasterizerStateDX11::RasterizerStateDX11( ID3D11Device2* pDevice )
    : m_pDevice( pDevice )
    , m_FrontFaceFillMode( FillMode::Solid )
    , m_BackFaceFillMode( FillMode::Solid )
    , m_CullMode( CullMode::Back )
    , m_FrontFace( FrontFace::CounterClockwise )
    , m_DepthBias( 0.0f )
    , m_SlopeBias( 0.0f )
    , m_BiasClamp( 0.0f )
    , m_DepthClipEnabled( true )
    , m_ScissorEnabled( false )
    , m_MultisampleEnabled( false )
    , m_AntialiasedLineEnabled( false )
    , m_ConservativeRasterization( false )
    , m_ForcedSampleCount( 0 )
    , m_StateDirty( true )
    , m_ViewportsDirty( true )
    , m_ScissorRectsDirty( true )
{
    m_pDevice->GetImmediateContext2( &m_pDeviceContext );

    m_Viewports.resize( 8, Viewport() );
    m_ScissorRects.resize( 8, Rect() );
}

RasterizerStateDX11::RasterizerStateDX11( const RasterizerStateDX11& copy )
    : m_pDevice( copy.m_pDevice )
    , m_pDeviceContext( copy.m_pDeviceContext )
    , m_d3dRects( copy.m_d3dRects )
    , m_d3dViewports( copy.m_d3dViewports )
    , m_FrontFaceFillMode( copy.m_FrontFaceFillMode )
    , m_BackFaceFillMode( copy.m_BackFaceFillMode )
    , m_CullMode( copy.m_CullMode )
    , m_FrontFace( copy.m_FrontFace )
    , m_DepthBias( copy.m_DepthBias )
    , m_SlopeBias( copy.m_SlopeBias )
    , m_BiasClamp( copy.m_BiasClamp )
    , m_DepthClipEnabled( copy.m_DepthClipEnabled )
    , m_ScissorEnabled( copy.m_ScissorEnabled )
    , m_MultisampleEnabled( copy.m_MultisampleEnabled )
    , m_AntialiasedLineEnabled( copy.m_AntialiasedLineEnabled )
    , m_ConservativeRasterization( copy.m_ConservativeRasterization )
    , m_ForcedSampleCount( copy.m_ForcedSampleCount )
    , m_ScissorRects( copy.m_ScissorRects )
    , m_Viewports( copy.m_Viewports )
    , m_StateDirty( true )
    , m_ViewportsDirty( false )
    , m_ScissorRectsDirty( false )
{}

RasterizerStateDX11::~RasterizerStateDX11()
{

}

const RasterizerStateDX11& RasterizerStateDX11::operator=( const RasterizerStateDX11& other )
{
    // avoid assignment to self.
    if ( this != &other )
    {
        m_d3dRects = other.m_d3dRects;
        m_d3dViewports = other.m_d3dViewports;

        m_FrontFaceFillMode = other.m_FrontFaceFillMode;
        m_BackFaceFillMode = other.m_BackFaceFillMode;

        m_CullMode = other.m_CullMode;

        m_FrontFace = other.m_FrontFace;

        m_DepthBias = other.m_DepthBias;
        m_SlopeBias = other.m_SlopeBias;
        m_BiasClamp = other.m_BiasClamp;

        m_DepthClipEnabled = other.m_DepthClipEnabled;
        m_ScissorEnabled = other.m_ScissorEnabled;

        m_MultisampleEnabled = other.m_MultisampleEnabled;
        m_AntialiasedLineEnabled = other.m_AntialiasedLineEnabled;

        m_ConservativeRasterization = other.m_ConservativeRasterization;

        m_ForcedSampleCount = other.m_ForcedSampleCount;

        m_ScissorRects = other.m_ScissorRects;
        m_Viewports = other.m_Viewports;

        m_StateDirty = true;
        m_ViewportsDirty = false;
        m_ScissorRectsDirty = false;
    }

    return *this;
}

void RasterizerStateDX11::SetFillMode( FillMode frontFace, FillMode backFace )
{
    m_FrontFaceFillMode = frontFace;
    m_BackFaceFillMode = backFace;
}

void RasterizerStateDX11::GetFillMode( FillMode& frontFace, FillMode& backFace ) const
{
    frontFace = m_FrontFaceFillMode;
    backFace = m_BackFaceFillMode;
}

void RasterizerStateDX11::SetCullMode( CullMode cullMode )
{
    m_CullMode = cullMode;
    m_StateDirty = true;
}

RasterizerState::CullMode RasterizerStateDX11::GetCullMode() const
{
    return m_CullMode;
}

void RasterizerStateDX11::SetFrontFacing( FrontFace frontFace )
{
    m_FrontFace = frontFace;
    m_StateDirty = true;
}

RasterizerState::FrontFace RasterizerStateDX11::GetFrontFacing() const
{
    return m_FrontFace;
}

void RasterizerStateDX11::SetDepthBias( float depthBias, float slopeBias, float biasClamp )
{
    m_DepthBias = depthBias;
    m_SlopeBias = slopeBias;
    m_BiasClamp = biasClamp;

    m_StateDirty = true;
}

void RasterizerStateDX11::GetDepthBias( float& depthBias, float& slopeBias, float& biasClamp ) const
{
    depthBias = m_DepthBias;
    slopeBias = m_SlopeBias;
    biasClamp = m_BiasClamp;
}

void RasterizerStateDX11::SetDepthClipEnabled( bool depthClipEnabled )
{
    m_DepthClipEnabled = depthClipEnabled;
    m_StateDirty = true;
}

bool RasterizerStateDX11::GetDepthClipEnabled() const
{
    return m_DepthClipEnabled;
}

void RasterizerStateDX11::SetViewport( const Viewport& viewport )
{
    m_Viewports[0] = viewport;
    m_ViewportsDirty = true;
}

void RasterizerStateDX11::SetViewports( const std::vector<Viewport>& viewports )
{
    m_Viewports = viewports;
    m_ViewportsDirty = true;
}

const std::vector<Viewport>& RasterizerStateDX11::GetViewports()
{
    return m_Viewports;
}

void RasterizerStateDX11::SetScissorEnabled( bool scissorEnable )
{
    m_ScissorEnabled = scissorEnable;
    m_StateDirty = true;
}

bool RasterizerStateDX11::GetScissorEnabled() const
{
    return m_ScissorEnabled;
}

void RasterizerStateDX11::SetScissorRect( const Rect& rect )
{
    m_ScissorRects[0] = rect;
    m_ScissorRectsDirty = true;
}

void RasterizerStateDX11::SetScissorRects( const std::vector<Rect>& rects )
{
    m_ScissorRects = rects;
    m_ScissorRectsDirty = true;
}

const std::vector<Rect>& RasterizerStateDX11::GetScissorRects() const
{
    return m_ScissorRects;
}

void RasterizerStateDX11::SetMultisampleEnabled( bool multisampleEnabled )
{
    m_MultisampleEnabled = multisampleEnabled;
    m_StateDirty = true;
}

bool RasterizerStateDX11::GetMultisampleEnabled() const
{
    return m_MultisampleEnabled;
}

void RasterizerStateDX11::SetAntialiasedLineEnable( bool antialiasedLineEnabled )
{
    m_AntialiasedLineEnabled = antialiasedLineEnabled;
    m_StateDirty = true;
}

bool RasterizerStateDX11::GetAntialiasedLineEnable() const
{
    return m_AntialiasedLineEnabled;
}

void RasterizerStateDX11::SetForcedSampleCount( uint8_t sampleCount )
{
    m_ForcedSampleCount = sampleCount;
    m_StateDirty = true;
}

uint8_t RasterizerStateDX11::GetForcedSampleCount()
{
    return m_ForcedSampleCount;
}

void RasterizerStateDX11::SetConservativeRasterizationEnabled( bool conservativeRasterizationEnabled )
{
    m_ConservativeRasterization = conservativeRasterizationEnabled;
}

bool RasterizerStateDX11::GetConservativeRasterizationEnabled() const
{
    // Currently, this implementation always returns false
    // because conservative rasterization is supported since DirectX 11.3 and 12.
    return false;
}

D3D11_FILL_MODE RasterizerStateDX11::TranslateFillMode( FillMode fillMode ) const
{
    D3D11_FILL_MODE result = D3D11_FILL_SOLID;
    switch ( fillMode )
    {
    case FillMode::Wireframe:
        result = D3D11_FILL_WIREFRAME;
        break;
    case FillMode::Solid:
        result = D3D11_FILL_SOLID;
        break;
    default:
        ReportError( "Unknown fill mode." );
        break;
    }

    return result;
}

D3D11_CULL_MODE RasterizerStateDX11::TranslateCullMode( CullMode cullMode ) const
{
    D3D11_CULL_MODE result = D3D11_CULL_BACK;
    switch ( cullMode )
    {
    case CullMode::None:
        result = D3D11_CULL_NONE;
        break;
    case CullMode::Front:
        result = D3D11_CULL_FRONT;
        break;
    case CullMode::Back:
        result = D3D11_CULL_BACK;
        break;
    case CullMode::FrontAndBack:
        // This mode is not supported in DX11.
        break;
    default:
        ReportError( "Unknown cull mode." );
        break;
    }

    return result;
}

bool RasterizerStateDX11::TranslateFrontFace( FrontFace frontFace ) const
{
    bool frontCounterClockwise = true;
    switch ( frontFace )
    {
    case FrontFace::Clockwise:
        frontCounterClockwise = false;
        break;
    case FrontFace::CounterClockwise:
        frontCounterClockwise = true;
        break;
    default:
        ReportError( "Unknown front face winding order." );
        break;
    }

    return frontCounterClockwise;
}

std::vector<D3D11_RECT> RasterizerStateDX11::TranslateRects( const std::vector<Rect>& rects ) const
{
    std::vector<D3D11_RECT> result( rects.size() );
    for ( unsigned int i = 0; i < rects.size(); i++ )
    {
        D3D11_RECT& d3dRect = result[i];
        const Rect& rect = rects[i];

        d3dRect.top = static_cast<LONG>( rect.Y + 0.5f );
        d3dRect.bottom = static_cast<LONG>( rect.Y + rect.Height + 0.5f );
        d3dRect.left = static_cast<LONG>( rect.X + 0.5f );
        d3dRect.right = static_cast<LONG>( rect.X + rect.Width + 0.5f );
    }

    return result;
}

std::vector<D3D11_VIEWPORT> RasterizerStateDX11::TranslateViewports( const std::vector<Viewport>& viewports ) const
{
    std::vector<D3D11_VIEWPORT> result( viewports.size() );
    for ( unsigned int i = 0; i < viewports.size(); i++ )
    {
        D3D11_VIEWPORT& d3dViewport = result[i];
        const Viewport& viewport = viewports[i];

        // I could probably do a reinterpret cast here...
        d3dViewport.TopLeftX = viewport.X;
        d3dViewport.TopLeftY = viewport.Y;
        d3dViewport.Width = viewport.Width;
        d3dViewport.Height = viewport.Height;
        d3dViewport.MinDepth = viewport.MinDepth;
        d3dViewport.MaxDepth = viewport.MaxDepth;
    }

    return result;
}

// Can only be invoked by the pipeline state
void RasterizerStateDX11::Bind()
{
    if ( m_StateDirty )
    {
        D3D11_RASTERIZER_DESC1 rasterizerDesc = {};

        rasterizerDesc.FillMode = TranslateFillMode( m_FrontFaceFillMode );
        rasterizerDesc.CullMode = TranslateCullMode( m_CullMode );
        rasterizerDesc.FrontCounterClockwise = TranslateFrontFace( m_FrontFace );
        rasterizerDesc.DepthBias = ( m_DepthBias < 0.0f ) ? static_cast<INT>( m_DepthBias - 0.5f ) : static_cast<INT>( m_DepthBias + 0.5f );
        rasterizerDesc.DepthBiasClamp = m_BiasClamp;
        rasterizerDesc.SlopeScaledDepthBias = m_SlopeBias;
        rasterizerDesc.DepthClipEnable = m_DepthClipEnabled;
        rasterizerDesc.ScissorEnable = m_ScissorEnabled;
        rasterizerDesc.MultisampleEnable = m_MultisampleEnabled;
        rasterizerDesc.AntialiasedLineEnable = m_AntialiasedLineEnabled;
        rasterizerDesc.ForcedSampleCount = m_ForcedSampleCount;

        if ( FAILED( m_pDevice->CreateRasterizerState1( &rasterizerDesc, &m_pRasterizerState ) ) )
        {
            ReportError( "Failed to create rasterizer state." );
        }

        m_StateDirty = false;
    }

    if ( m_ScissorRectsDirty )
    {
        m_d3dRects = TranslateRects( m_ScissorRects );
        m_ScissorRectsDirty = false;
    }

    if ( m_ViewportsDirty )
    {
        m_d3dViewports = TranslateViewports( m_Viewports );
        m_ViewportsDirty = false;
    }

    m_pDeviceContext->RSSetViewports( (UINT)m_d3dViewports.size(), m_d3dViewports.data() );
    m_pDeviceContext->RSSetScissorRects( (UINT)m_d3dRects.size(), m_d3dRects.data() );
    m_pDeviceContext->RSSetState( m_pRasterizerState.Get() );
}