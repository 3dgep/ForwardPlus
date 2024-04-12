#include <EnginePCH.h>

#include "DepthStencilStateDX11.h"

DepthStencilStateDX11::DepthStencilStateDX11( ID3D11Device2* pDevice )
    : m_pDevice( pDevice )
    , m_bDirty( true )
{
    assert( pDevice );
    m_pDevice->GetImmediateContext2( &m_pDeviceContext );
}

DepthStencilStateDX11::DepthStencilStateDX11( const DepthStencilStateDX11& copy )
    : m_pDevice( copy.m_pDevice )
    , m_pDeviceContext( copy.m_pDeviceContext )
    , m_DepthMode( copy.m_DepthMode )
    , m_StencilMode( copy.m_StencilMode )
    , m_bDirty( true )
{}

DepthStencilStateDX11::~DepthStencilStateDX11()
{

}

const DepthStencilStateDX11& DepthStencilStateDX11::operator=( const DepthStencilStateDX11& other )
{
    if ( this != &other )
    {
        m_pDevice = other.m_pDevice;
        m_pDeviceContext = other.m_pDeviceContext;
        m_DepthMode = other.m_DepthMode;
        m_StencilMode = other.m_StencilMode;
        m_bDirty = true;
    }

    return *this;
}

void DepthStencilStateDX11::SetDepthMode( const DepthMode& depthMode )
{
    m_DepthMode = depthMode;
    m_bDirty = true;
}

const DepthStencilState::DepthMode& DepthStencilStateDX11::GetDepthMode() const
{
    return m_DepthMode;
}

void DepthStencilStateDX11::SetStencilMode( const StencilMode& stencilMode )
{
    m_StencilMode = stencilMode;
    m_bDirty = true;
}

const DepthStencilState::StencilMode& DepthStencilStateDX11::GetStencilMode() const
{
    return m_StencilMode;
}

D3D11_DEPTH_WRITE_MASK DepthStencilStateDX11::TranslateDepthWriteMask( DepthWrite depthWrite ) const
{
    D3D11_DEPTH_WRITE_MASK result = D3D11_DEPTH_WRITE_MASK_ALL;

    switch ( depthWrite )
    {
    case DepthWrite::Enable:
        result = D3D11_DEPTH_WRITE_MASK_ALL;
        break;
    case DepthWrite::Disable:
        result = D3D11_DEPTH_WRITE_MASK_ZERO;
        break;
    default:
        ReportError( "Unknown depth write mask." );
        break;
    }

    return result;
}

D3D11_COMPARISON_FUNC DepthStencilStateDX11::TranslateCompareFunc( CompareFunction compareFunc ) const
{
    D3D11_COMPARISON_FUNC result = D3D11_COMPARISON_LESS;

    switch ( compareFunc )
    {
    case CompareFunction::Never:
        result = D3D11_COMPARISON_NEVER;
        break;
    case CompareFunction::Less:
        result = D3D11_COMPARISON_LESS;
        break;
    case CompareFunction::Equal:
        result = D3D11_COMPARISON_EQUAL;
        break;
    case CompareFunction::LessOrEqual:
        result = D3D11_COMPARISON_LESS_EQUAL;
        break;
    case CompareFunction::Greater:
        result = D3D11_COMPARISON_GREATER;
        break;
    case CompareFunction::NotEqual:
        result = D3D11_COMPARISON_NOT_EQUAL;
        break;
    case CompareFunction::GreaterOrEqual:
        result = D3D11_COMPARISON_GREATER_EQUAL;
        break;
    case CompareFunction::Always:
        result = D3D11_COMPARISON_ALWAYS;
        break;
    default:
        ReportError( "Unknown compare function." );
        break;
    }

    return result;
}

D3D11_STENCIL_OP DepthStencilStateDX11::TranslateStencilOperation( StencilOperation stencilOperation ) const
{
    D3D11_STENCIL_OP result = D3D11_STENCIL_OP_KEEP;

    switch ( stencilOperation )
    {
    case StencilOperation::Keep:
        result = D3D11_STENCIL_OP_KEEP;
        break;
    case StencilOperation::Zero:
        result = D3D11_STENCIL_OP_ZERO;
        break;
    case StencilOperation::Reference:
        result = D3D11_STENCIL_OP_REPLACE;
        break;
    case StencilOperation::IncrementClamp:
        result = D3D11_STENCIL_OP_INCR_SAT;
        break;
    case StencilOperation::DecrementClamp:
        result = D3D11_STENCIL_OP_DECR_SAT;
        break;
    case StencilOperation::Invert:
        result = D3D11_STENCIL_OP_INVERT;
        break;
    case StencilOperation::IncrementWrap:
        result = D3D11_STENCIL_OP_INCR;
        break;
    case StencilOperation::DecrementWrap:
        result = D3D11_STENCIL_OP_DECR;
        break;
    default:
        ReportError( "Unknown stencil operation." );
        break;
    }

    return result;
}

D3D11_DEPTH_STENCILOP_DESC DepthStencilStateDX11::TranslateFaceOperation( FaceOperation faceOperation ) const
{
    D3D11_DEPTH_STENCILOP_DESC result;

    result.StencilFailOp = TranslateStencilOperation( faceOperation.StencilFail );
    result.StencilDepthFailOp = TranslateStencilOperation( faceOperation.StencilPassDepthFail );
    result.StencilPassOp = TranslateStencilOperation( faceOperation.StencilDepthPass );
    result.StencilFunc = TranslateCompareFunc( faceOperation.StencilFunction );

    return result;
}

D3D11_DEPTH_STENCIL_DESC DepthStencilStateDX11::TranslateDepthStencilState( const DepthMode& depthMode, const StencilMode& stencilMode ) const
{
    D3D11_DEPTH_STENCIL_DESC result;

    result.DepthEnable = depthMode.DepthEnable;
    result.DepthWriteMask = TranslateDepthWriteMask( depthMode.DepthWriteMask );
    result.DepthFunc = TranslateCompareFunc( depthMode.DepthFunction );
    result.StencilEnable = stencilMode.StencilEnabled;
    result.StencilReadMask = stencilMode.ReadMask;
    result.StencilWriteMask = stencilMode.WriteMask;
    result.FrontFace = TranslateFaceOperation( stencilMode.FrontFace );
    result.BackFace = TranslateFaceOperation( stencilMode.BackFace );

    return result;
}

void DepthStencilStateDX11::Bind()
{
    if ( m_bDirty )
    {
        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = TranslateDepthStencilState( m_DepthMode, m_StencilMode );

        if ( FAILED( m_pDevice->CreateDepthStencilState( &depthStencilDesc, &m_pDepthStencilState ) ) )
        {
            ReportError( "Failed to create depth stencil state." );
        }

        m_bDirty = false;
    }

    m_pDeviceContext->OMSetDepthStencilState( m_pDepthStencilState.Get(), m_StencilMode.StencilReference );
}