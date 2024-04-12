#pragma once

#include <ConstantBuffer.h>

class ConstantBufferDX11 : public ConstantBuffer
{
public:
    ConstantBufferDX11( ID3D11Device2* pDevice, size_t size );
    virtual ~ConstantBufferDX11();

    virtual bool Bind( unsigned int id, Shader::ShaderType shaderType, ShaderParameter::Type parameterType );
    virtual void UnBind( unsigned int id, Shader::ShaderType shaderType, ShaderParameter::Type parameterType );

    virtual void Copy( std::shared_ptr<ConstantBuffer> other );

protected:
    virtual void Copy( std::shared_ptr<Buffer> other );
    void Set( const void* data, size_t size );

private:
    Microsoft::WRL::ComPtr<ID3D11Device2> m_pDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext2> m_pDeviceContext;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_pBuffer;

    size_t  m_BufferSize;
};