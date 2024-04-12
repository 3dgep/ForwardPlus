#pragma once

#include <Viewport.h>
#include <Rect.h>

#include <RasterizerState.h>

class RasterizerStateDX11 : public RasterizerState
{
public:

    RasterizerStateDX11( ID3D11Device2* pDevice );
    RasterizerStateDX11( const RasterizerStateDX11& copy );
    virtual ~RasterizerStateDX11();

    const RasterizerStateDX11& operator=( const RasterizerStateDX11& other );

    virtual void SetFillMode( FillMode frontFace = FillMode::Solid, FillMode backFace = FillMode::Solid );
    virtual void GetFillMode( FillMode& frontFace, FillMode& backFace ) const;

    virtual void SetCullMode( CullMode cullMode = CullMode::Back );
    virtual CullMode GetCullMode() const;

    virtual void SetFrontFacing( FrontFace frontFace = FrontFace::CounterClockwise );
    virtual FrontFace GetFrontFacing() const;

    virtual void SetDepthBias( float depthBias = 0.0f, float slopeBias = 0.0f, float biasClamp = 0.0f );
    virtual void GetDepthBias( float& depthBias, float& slopeBias, float& biasClamp ) const;

    virtual void SetDepthClipEnabled( bool depthClipEnabled = true );
    virtual bool GetDepthClipEnabled() const;

    virtual void SetViewport( const Viewport& viewport );
    virtual void SetViewports( const std::vector<Viewport>& viewports );
    virtual const std::vector<Viewport>& GetViewports();

    virtual void SetScissorEnabled( bool scissorEnable = false );
    virtual bool GetScissorEnabled() const;

    virtual void SetScissorRect( const Rect& rect );
    virtual void SetScissorRects( const std::vector<Rect>& rects );
    virtual const std::vector<Rect>& GetScissorRects() const;

    virtual void SetMultisampleEnabled( bool multisampleEnabled = false );
    virtual bool GetMultisampleEnabled() const;

    virtual void SetAntialiasedLineEnable( bool antialiasedLineEnabled );
    virtual bool GetAntialiasedLineEnable() const;

    virtual void SetForcedSampleCount( uint8_t sampleCount );
    virtual uint8_t GetForcedSampleCount();

    virtual void SetConservativeRasterizationEnabled( bool conservativeRasterizationEnabled = false );
    virtual bool GetConservativeRasterizationEnabled() const;

    // Can only be invoked by the pipeline state
    virtual void Bind();
protected:
    
    D3D11_FILL_MODE TranslateFillMode( FillMode fillMode ) const;
    D3D11_CULL_MODE TranslateCullMode( CullMode cullMode ) const;
    bool TranslateFrontFace( FrontFace frontFace ) const;

    std::vector<D3D11_RECT> TranslateRects( const std::vector<Rect>& rects ) const;
    std::vector<D3D11_VIEWPORT> TranslateViewports( const std::vector<Viewport>& viewports ) const;

private:
    Microsoft::WRL::ComPtr<ID3D11Device2> m_pDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext2> m_pDeviceContext;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState1> m_pRasterizerState;

    std::vector<D3D11_RECT> m_d3dRects;
    std::vector<D3D11_VIEWPORT> m_d3dViewports;

    FillMode m_FrontFaceFillMode;
    FillMode m_BackFaceFillMode;

    CullMode m_CullMode;

    FrontFace m_FrontFace;

    float m_DepthBias;
    float m_SlopeBias;
    float m_BiasClamp;

    bool m_DepthClipEnabled;
    bool m_ScissorEnabled;

    bool m_MultisampleEnabled;
    bool m_AntialiasedLineEnabled;

    bool m_ConservativeRasterization;

    uint8_t m_ForcedSampleCount;

    typedef std::vector<Rect> RectList;
    RectList m_ScissorRects;

    typedef std::vector<Viewport> ViewportList;
    ViewportList m_Viewports;

    // Set to true when the rasterizer state needs to be recreated.
    bool m_StateDirty;
    bool m_ViewportsDirty;
    bool m_ScissorRectsDirty;
};