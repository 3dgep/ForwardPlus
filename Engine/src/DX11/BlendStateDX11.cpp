#include <EnginePCH.h>

#include "BlendStateDX11.h"

BlendStateDX11::BlendStateDX11( ID3D11Device2* pDevice )
    : m_pDevice( pDevice )
    , m_bAlphaToCoverageEnabled( false )
    , m_bIndependentBlendEnabled( false )
    , m_SampleMask( 0xffffffff )
    , m_ConstBlendFactor( 1 )
    , m_bDirty( true )
{
    if ( m_pDevice )
    {
        m_pDevice->GetImmediateContext2( &m_pDeviceContext );
    }

    m_BlendModes.resize( 8, BlendMode() );
}

BlendStateDX11::BlendStateDX11( const BlendStateDX11& copy )
    : m_pDevice( copy.m_pDevice )
    , m_pDeviceContext( copy.m_pDeviceContext )
    , m_BlendModes( copy.m_BlendModes )
    , m_bAlphaToCoverageEnabled( copy.m_bAlphaToCoverageEnabled )
    , m_bIndependentBlendEnabled( copy.m_bIndependentBlendEnabled )
    , m_SampleMask( copy.m_SampleMask )
    , m_ConstBlendFactor( copy.m_ConstBlendFactor )
    , m_bDirty( true )
{

}

BlendStateDX11::~BlendStateDX11()
{

}

const BlendStateDX11& BlendStateDX11::operator=( const BlendStateDX11& other )
{
    // Avoid copy to self..
    if ( this != &other )
    {
        m_BlendModes = other.m_BlendModes;
        m_bAlphaToCoverageEnabled = other.m_bAlphaToCoverageEnabled;
        m_bIndependentBlendEnabled = other.m_bIndependentBlendEnabled;
        m_SampleMask = other.m_SampleMask;
        m_ConstBlendFactor = other.m_ConstBlendFactor;

        m_bDirty = true;
    }

    return *this;
}

void BlendStateDX11::SetBlendMode( const BlendState::BlendMode& blendMode )
{
    m_BlendModes[0] = blendMode;
    m_bDirty = true;
}

void BlendStateDX11::SetBlendModes( const std::vector<BlendMode>& blendModes )
{
    m_BlendModes = blendModes;
    m_bDirty;
}

const std::vector<BlendState::BlendMode>& BlendStateDX11::GetBlendModes() const
{
    return m_BlendModes;
}

void BlendStateDX11::SetAlphaCoverage( bool enabled )
{
    if ( m_bAlphaToCoverageEnabled != enabled )
    {
        m_bAlphaToCoverageEnabled = enabled;
        m_bDirty = true;
    }

}

bool BlendStateDX11::GetAlphaCoverage() const
{
    return m_bAlphaToCoverageEnabled;
}

void BlendStateDX11::SetIndependentBlend( bool enabled )
{
    if ( m_bAlphaToCoverageEnabled != enabled )
    {
        m_bAlphaToCoverageEnabled = enabled;
        m_bDirty = true;
    }
}

bool BlendStateDX11::GetIndependentBlend() const
{
    return m_bIndependentBlendEnabled;
}

void BlendStateDX11::SetConstBlendFactor( const glm::vec4& constantBlendFactor )
{
    m_ConstBlendFactor = constantBlendFactor;
    // No need to set the dirty flag as this value is not used to create the blend state object.
    // It is only used when activating the blend state of the output merger.
}

const glm::vec4& BlendStateDX11::GetConstBlendFactor() const
{
    return m_ConstBlendFactor;
}

void BlendStateDX11::SetSampleMask( uint32_t sampleMask )
{
    m_SampleMask = sampleMask;
    // No need to set the dirty flag as this value is not used to create the blend state object.
    // It is only used when activating the blend state of the output merger.
}

uint32_t BlendStateDX11::GetSampleMask() const
{
    return m_SampleMask;
}

D3D11_BLEND BlendStateDX11::TranslateBlendFactor( BlendState::BlendFactor blendFactor ) const
{
    D3D11_BLEND result = D3D11_BLEND_ONE;

    switch ( blendFactor )
    {
    case BlendFactor::Zero:
        result = D3D11_BLEND_ZERO;
        break;
    case BlendFactor::One:
        result = D3D11_BLEND_ONE;
        break;
    case BlendFactor::SrcColor:
        result = D3D11_BLEND_SRC_COLOR;
        break;
    case BlendFactor::OneMinusSrcColor:
        result = D3D11_BLEND_INV_SRC_COLOR;
        break;
    case BlendFactor::DstColor:
        result = D3D11_BLEND_DEST_COLOR;
        break;
    case BlendFactor::OneMinusDstColor:
        result = D3D11_BLEND_INV_DEST_COLOR;
        break;
    case BlendFactor::SrcAlpha:
        result = D3D11_BLEND_SRC_ALPHA;
        break;
    case BlendFactor::OneMinusSrcAlpha:
        result = D3D11_BLEND_INV_SRC_ALPHA;
        break;
    case BlendFactor::DstAlpha:
        result = D3D11_BLEND_DEST_ALPHA;
        break;
    case BlendFactor::OneMinusDstAlpha:
        result = D3D11_BLEND_INV_DEST_ALPHA;
        break;
    case BlendFactor::SrcAlphaSat:
        result = D3D11_BLEND_SRC_ALPHA_SAT;
        break;
    case BlendFactor::ConstBlendFactor:
        result = D3D11_BLEND_BLEND_FACTOR;
        break;
    case BlendFactor::OneMinusBlendFactor:
        result = D3D11_BLEND_INV_BLEND_FACTOR;
        break;
    case BlendFactor::Src1Color:
        result = D3D11_BLEND_SRC1_COLOR;
        break;
    case BlendFactor::OneMinusSrc1Color:
        result = D3D11_BLEND_INV_SRC1_COLOR;
        break;
    case BlendFactor::Src1Alpha:
        result = D3D11_BLEND_INV_SRC1_ALPHA;
        break;
    case BlendFactor::OneMinusSrc1Alpha:
        result = D3D11_BLEND_INV_SRC1_ALPHA;
        break;
    default:
        ReportError( "Unknown blend factor." );
        break;
    }

    return result;
}

D3D11_BLEND_OP BlendStateDX11::TranslateBlendOp( BlendState::BlendOperation blendOperation ) const
{
    D3D11_BLEND_OP result = D3D11_BLEND_OP_ADD;
    switch ( blendOperation )
    {
    case BlendOperation::Add:
        result = D3D11_BLEND_OP_ADD;
        break;
    case BlendOperation::Subtract:
        result = D3D11_BLEND_OP_SUBTRACT;
        break;
    case BlendOperation::ReverseSubtract:
        result = D3D11_BLEND_OP_REV_SUBTRACT;
        break;
    case BlendOperation::Min:
        result = D3D11_BLEND_OP_MIN;
        break;
    case BlendOperation::Max:
        result = D3D11_BLEND_OP_MAX;
        break;
    default:
        ReportError( "Unknown blend operation." );
        break;
    }

    return result;
}

UINT8 BlendStateDX11::TranslateWriteMask( bool red, bool green, bool blue, bool alpha ) const
{
    UINT8 writeMask = 0;
    if ( red )
    {
        writeMask |= D3D11_COLOR_WRITE_ENABLE_RED;
    }
    if ( green )
    {
        writeMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
    }
    if ( blue )
    {
        writeMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
    }
    if ( alpha )
    {
        writeMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
    }

    return writeMask;
}

D3D11_LOGIC_OP BlendStateDX11::TranslateLogicOperator( LogicOperator logicOp ) const
{
    D3D11_LOGIC_OP result = D3D11_LOGIC_OP_NOOP;

    switch ( logicOp )
    {
    case LogicOperator::None:
        result = D3D11_LOGIC_OP_NOOP;
        break;
    case LogicOperator::Clear:
        result = D3D11_LOGIC_OP_CLEAR;
        break;
    case LogicOperator::Set:
        result = D3D11_LOGIC_OP_SET;
        break;
    case LogicOperator::Copy:
        result = D3D11_LOGIC_OP_SET;
        break;
    case LogicOperator::CopyInverted:
        result = D3D11_LOGIC_OP_COPY_INVERTED;
        break;
    case LogicOperator::Invert:
        result = D3D11_LOGIC_OP_INVERT;
        break;
    case LogicOperator::And:
        result = D3D11_LOGIC_OP_AND;
        break;
    case LogicOperator::Nand:
        result = D3D11_LOGIC_OP_NAND;
        break;
    case LogicOperator::Or:
        result = D3D11_LOGIC_OP_OR;
        break;
    case LogicOperator::Nor:
        result = D3D11_LOGIC_OP_NOR;
        break;
    case LogicOperator::Xor:
        result = D3D11_LOGIC_OP_XOR;
        break;
    case LogicOperator::Equiv:
        result = D3D11_LOGIC_OP_EQUIV;
        break;
    case LogicOperator::AndReverse:
        result = D3D11_LOGIC_OP_AND_REVERSE;
        break;
    case LogicOperator::AndInverted:
        result = D3D11_LOGIC_OP_AND_INVERTED;
        break;
    case LogicOperator::OrReverse:
        result = D3D11_LOGIC_OP_OR_REVERSE;
        break;
    case LogicOperator::OrInverted:
        result = D3D11_LOGIC_OP_OR_INVERTED;
        break;
    default:
        break;
    }

    return result;
}

void BlendStateDX11::Bind()
{
    if ( m_bDirty )
    {
        // (Re)create the blend state object.
        D3D11_BLEND_DESC1 blendDesc;

        blendDesc.AlphaToCoverageEnable = m_bAlphaToCoverageEnabled;
        blendDesc.IndependentBlendEnable = m_bIndependentBlendEnabled;
        for ( unsigned int i = 0; i < 8 && i < m_BlendModes.size(); i++ )
        {
            D3D11_RENDER_TARGET_BLEND_DESC1& rtBlendDesc = blendDesc.RenderTarget[i];
            BlendMode& blendMode = m_BlendModes[i];

            rtBlendDesc.BlendEnable = blendMode.BlendEnabled;
            rtBlendDesc.LogicOpEnable = blendMode.LogicOpEnabled;
            rtBlendDesc.SrcBlend = TranslateBlendFactor( blendMode.SrcFactor );
            rtBlendDesc.DestBlend = TranslateBlendFactor( blendMode.DstFactor );
            rtBlendDesc.BlendOp = TranslateBlendOp( blendMode.BlendOp );
            rtBlendDesc.SrcBlendAlpha = TranslateBlendFactor( blendMode.SrcAlphaFactor );
            rtBlendDesc.DestBlendAlpha = TranslateBlendFactor( blendMode.DstAlphaFactor );
            rtBlendDesc.BlendOpAlpha = TranslateBlendOp( blendMode.AlphaOp );
            rtBlendDesc.LogicOp = TranslateLogicOperator( blendMode.LogicOp );
            rtBlendDesc.RenderTargetWriteMask = TranslateWriteMask( blendMode.WriteRed, blendMode.WriteGreen, blendMode.WriteBlue, blendMode.WriteAlpha );
        }

        m_pDevice->CreateBlendState1( &blendDesc, &m_pBlendState );

        m_bDirty = false;
    }

    // Now activate the blend state:
    m_pDeviceContext->OMSetBlendState( m_pBlendState.Get(), glm::value_ptr( m_ConstBlendFactor ), m_SampleMask );
}

