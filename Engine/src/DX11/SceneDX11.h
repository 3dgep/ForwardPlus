#pragma once

#include "../SceneBase.h"

class RenderDeviceDX11;

class SceneDX11 : public SceneBase
{
public:
    SceneDX11( RenderDeviceDX11& pDevice );
    virtual ~SceneDX11();
protected:
    virtual std::shared_ptr<Buffer> CreateFloatVertexBuffer( const float* data, unsigned int count, unsigned int stride ) const;
    virtual std::shared_ptr<Buffer> CreateUIntIndexBuffer( const unsigned int* data, unsigned int count ) const;

    virtual std::shared_ptr<Mesh> CreateMesh() const;
    virtual std::shared_ptr<Material> CreateMaterial() const;
    virtual std::shared_ptr<Texture> CreateTexture( const std::wstring& fileName ) const;
    virtual std::shared_ptr<Texture> CreateTexture2D( uint16_t width, uint16_t height );
    virtual std::shared_ptr<Texture> GetDefaultTexture();

private:
    RenderDeviceDX11& m_Device;

    Microsoft::WRL::ComPtr<ID3D11Device2> m_pDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext2> m_pContext;

};