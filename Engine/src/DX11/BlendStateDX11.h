#pragma once

#include <BlendState.h>

class BlendStateDX11 : public BlendState
{
public:
    BlendStateDX11( ID3D11Device2* pDevice );
    BlendStateDX11( const BlendStateDX11& copy );

    virtual ~BlendStateDX11();

    const BlendStateDX11& operator=( const BlendStateDX11& other );

    /**
    * Set a blend mode on a back buffer index.
    * The maximum number of back buffers that can be simultaneously bound is
    * specific to the rendering API. For DX11 and DX12, the maximum is 8. In OpenGL,
    * the minimum required is 8.
    */
    virtual void SetBlendMode( const BlendMode& blendMode );
    virtual void SetBlendModes( const std::vector<BlendMode>& blendModes );
    virtual const std::vector<BlendMode>& GetBlendModes() const;

    /**
    * Set a constant blend factor that is used when the BlendFactor is set to ConstBlendFactor.
    */
    virtual void SetConstBlendFactor( const glm::vec4& constantBlendFactor );
    virtual const glm::vec4& GetConstBlendFactor() const;

    /**
    * The sample mask determines which samples get updated in all the active render targets.
    * When multisample anti-aliasing is enabled, the pixel color is determined at 2, 4, 8 or 16
    * subpixels within a single pixel and the final result is blended to achieve
    * an anti-aliased effect. The sample mask determines which subpixel locations are
    * used to determine the final pixel color.
    * The default value is 0xffffffff which enables all subpixel locations to be used
    * for the final color. Note that a 32-bit sample mask allows you to mask 32 samples. As far
    * as I know at the time of this writing, only 16 samples are supported so only
    * bits 0 - 15 in the sample mask are considered (but future API implementations may
    * support up to 32 samples per pixel.. or more!? If that ever happens, I'll have to change this
    * to a 64-bit mask...)
    * @see https://msdn.microsoft.com/en-us/library/windows/desktop/ff476462%28v=vs.85%29.aspx
    * @see https://www.opengl.org/sdk/docs/man/html/glSampleMaski.xhtml
    */
    virtual void SetSampleMask( uint32_t sampleMask );
    virtual uint32_t GetSampleMask() const;

    /**
    * Whether to enable alpha coverage.
    * @see https://msdn.microsoft.com/en-us/library/windows/desktop/bb205072(v=vs.85).aspx#Alpha_To_Coverage
    * @see https://www.opengl.org/sdk/docs/man4/html/glSampleCoverage.xhtml
    */
    virtual void SetAlphaCoverage( bool enabled );
    virtual bool GetAlphaCoverage() const;

    /**
    * Enable independent blend modes for render target color buffers.
    * If set to true then the BlendMode settings for each color buffer bound
    * to the current render target will be used. If set to false, only the BlendMode
    * value set at index 0 will be used to blend all currently bound color buffers.
    */
    virtual void SetIndependentBlend( bool enabled );
    virtual bool GetIndependentBlend() const;

    // Can only be bound by the pipeline state.
    virtual void Bind();

protected:

    D3D11_BLEND TranslateBlendFactor( BlendState::BlendFactor blendFactor ) const;
    D3D11_BLEND_OP TranslateBlendOp( BlendState::BlendOperation blendOperation ) const;
    UINT8 TranslateWriteMask( bool red, bool green, bool blue, bool alpha ) const;
    D3D11_LOGIC_OP TranslateLogicOperator( LogicOperator logicOp ) const;

private:
    Microsoft::WRL::ComPtr< ID3D11Device2 > m_pDevice;
    Microsoft::WRL::ComPtr< ID3D11DeviceContext2 > m_pDeviceContext;
    Microsoft::WRL::ComPtr< ID3D11BlendState1 > m_pBlendState;

    typedef std::vector<BlendMode> BlendModeList;
    // A vector of blend modes. One for each render target view that is bound
    // to the output merger stage.
    BlendModeList m_BlendModes;

    bool m_bAlphaToCoverageEnabled;
    bool m_bIndependentBlendEnabled;
    uint32_t m_SampleMask;

    glm::vec4 m_ConstBlendFactor;

    // Set to true if we need to recreate the blend state object.
    bool m_bDirty;
};