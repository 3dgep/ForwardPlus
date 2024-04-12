#include <EnginePCH.h>

#include "TextureDX11.h"
#include "StructuredBufferDX11.h"

#include "RenderTargetDX11.h"

RenderTargetDX11::RenderTargetDX11( ID3D11Device2* pDevice )
    : m_pDevice( pDevice )
    , m_Width( 0 )
    , m_Height( 0 )
    , m_bCheckValidity( false )
{
    m_pDevice->GetImmediateContext2( &m_pDeviceContext );
    m_Textures.resize( (size_t)RenderTarget::AttachmentPoint::NumAttachmentPoints + 1 );
    m_StructuredBuffers.resize( 8 );
}

RenderTargetDX11::~RenderTargetDX11()
{

}

void RenderTargetDX11::AttachTexture( AttachmentPoint attachment, std::shared_ptr<Texture> texture )
{
    std::shared_ptr<TextureDX11> textureDX11 = std::dynamic_pointer_cast<TextureDX11>( texture );    
    m_Textures[(uint8_t)attachment] = textureDX11;

    // Next time the render target is "bound", check that it is valid.
    m_bCheckValidity = true;
}

std::shared_ptr<Texture> RenderTargetDX11::GetTexture( AttachmentPoint attachment )
{
    return m_Textures[(uint8_t)attachment];
}


void RenderTargetDX11::Clear( AttachmentPoint attachment, ClearFlags clearFlags, const glm::vec4& color, float depth, uint8_t stencil )
{
    std::shared_ptr<TextureDX11> texture = m_Textures[(uint8_t)attachment];
    if ( texture )
    {
        texture->Clear( clearFlags, color, depth, stencil );
    }
}

void RenderTargetDX11::Clear( ClearFlags clearFlags, const glm::vec4& color, float depth, uint8_t stencil )
{
    for ( uint8_t i = 0; i < (uint8_t)AttachmentPoint::NumAttachmentPoints; ++i )
    {
        Clear( (AttachmentPoint)i, clearFlags, color, depth, stencil );
    }
}

void RenderTargetDX11::GenerateMipMaps()
{
    for ( auto texture : m_Textures )
    {
        if ( texture )
        {
            texture->GenerateMipMaps();
        }
    }
}

void RenderTargetDX11::AttachStructuredBuffer( uint8_t slot, std::shared_ptr<StructuredBuffer> rwBuffer )
{
    std::shared_ptr<StructuredBufferDX11> rwbufferDX11 = std::dynamic_pointer_cast<StructuredBufferDX11>( rwBuffer );
    m_StructuredBuffers[slot] = rwbufferDX11;

    // Next time the render target is "bound", check that it is valid.
    m_bCheckValidity = true;
}

std::shared_ptr<StructuredBuffer> RenderTargetDX11::GetStructuredBuffer( uint8_t slot )
{
    if ( slot < m_StructuredBuffers.size() )
    {
        return m_StructuredBuffers[slot];
    }
    return std::shared_ptr<StructuredBuffer>();
}


void RenderTargetDX11::Resize( uint16_t width, uint16_t height )
{
    if ( m_Width != width || m_Height != height )
    {
        m_Width = glm::max<uint16_t>( width, 1 );
        m_Height = glm::max<uint16_t>( height, 1 );
        // Resize the attached textures.
        for ( auto texture : m_Textures )
        {
            if ( texture )
            {
                texture->Resize( m_Width, m_Height );
            }
        }
    }
}

void RenderTargetDX11::Bind()
{
    if ( m_bCheckValidity )
    {
        if ( !IsValid() )
        {
            ReportError( "Invalid render target." );
        }
        m_bCheckValidity = false;
    }

    ID3D11RenderTargetView* renderTargetViews[8];
    UINT numRTVs = 0;

    for ( uint8_t i = 0; i < 8; i++ )
    {
        std::shared_ptr<TextureDX11> texture = m_Textures[i];
        if ( texture )
        {
            renderTargetViews[numRTVs++] = texture->GetRenderTargetView();
        }
    }

    ID3D11UnorderedAccessView* uavViews[8];
    UINT uavStartSlot = numRTVs;
    UINT numUAVs = 0;

    for ( uint8_t i = 0; i < 8; i++ )
    {
        std::shared_ptr<StructuredBufferDX11> rwbuffer = m_StructuredBuffers[i];
        if ( rwbuffer )
        {
            uavViews[numUAVs++] = rwbuffer->GetUnorderedAccessView();
        }
    }
    
    ID3D11DepthStencilView* depthStencilView = nullptr;
    std::shared_ptr<TextureDX11> depthTexture = m_Textures[(uint8_t)AttachmentPoint::Depth];
    std::shared_ptr<TextureDX11> depthStencilTexture = m_Textures[(uint8_t)AttachmentPoint::DepthStencil];

    if ( depthTexture )
    {
        depthStencilView = depthTexture->GetDepthStencilView();
    }
    else if ( depthStencilTexture )
    {
        depthStencilView = depthStencilTexture->GetDepthStencilView();
    }

    m_pDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews( numRTVs, renderTargetViews, depthStencilView, uavStartSlot, numUAVs, uavViews, nullptr );
}

void RenderTargetDX11::UnBind()
{
    m_pDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews( 0, nullptr, nullptr, 0, 0, nullptr, nullptr );
}

bool RenderTargetDX11::IsValid() const
{
    UINT numRTV = 0;
    int width = -1;
    int height = -1;

    for ( auto texture : m_Textures )
    {
        if ( texture )
        {
            if ( texture->GetRenderTargetView() ) ++numRTV;

            if ( width == -1 || height == -1 )
            {
                width = texture->GetWidth();
                height = texture->GetHeight();
            }
            else
            {
                if ( texture->GetWidth() != width || texture->GetHeight() != height )
                {
                    return false;
                }
            }
        }
    }

    UINT numUAV = 0;
    for ( auto rwBuffer : m_StructuredBuffers )
    {
        if ( rwBuffer )
        {
            ++numUAV;
        }
    }

    if ( numRTV + numUAV > 8 )
    {
        return false;
    }

    return true;
}


