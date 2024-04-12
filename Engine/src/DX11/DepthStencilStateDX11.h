#pragma once

#include <DepthStencilState.h>

class DepthStencilStateDX11 : public DepthStencilState
{
public:
    DepthStencilStateDX11( ID3D11Device2* pDevice );
    DepthStencilStateDX11( const DepthStencilStateDX11& copy );

    virtual ~DepthStencilStateDX11();

    const DepthStencilStateDX11& operator=( const DepthStencilStateDX11& other );

    virtual void SetDepthMode( const DepthMode& depthMode );
    virtual const DepthMode& GetDepthMode() const;

    virtual void SetStencilMode( const StencilMode& stencilMode );
    virtual const StencilMode& GetStencilMode() const;

    // Can only be called by the pipeline state.
    void Bind();
protected:
    
    D3D11_DEPTH_WRITE_MASK TranslateDepthWriteMask( DepthWrite depthWrite ) const;
    D3D11_COMPARISON_FUNC TranslateCompareFunc( CompareFunction compareFunc ) const;
    D3D11_STENCIL_OP TranslateStencilOperation( StencilOperation stencilOperation ) const;
    D3D11_DEPTH_STENCILOP_DESC TranslateFaceOperation( FaceOperation faceOperation ) const;
    D3D11_DEPTH_STENCIL_DESC TranslateDepthStencilState( const DepthMode& depthMode, const StencilMode& stencilMode ) const;

private:
    Microsoft::WRL::ComPtr<ID3D11Device2> m_pDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext2> m_pDeviceContext;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_pDepthStencilState;

    DepthMode m_DepthMode;
    StencilMode m_StencilMode;

    bool m_bDirty;
};