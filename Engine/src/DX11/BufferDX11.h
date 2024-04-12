#pragma once

#include <Buffer.h>

class BufferDX11 : public Buffer
{
public:
    BufferDX11( ID3D11Device2* pDevice, UINT bindFlags, const void* data, size_t count, UINT stride );
    ~BufferDX11();

    // Bind the buffer to a particular attribute ID or slot
    virtual bool Bind( unsigned int id, Shader::ShaderType shaderType, ShaderParameter::Type parameterType );
    virtual void UnBind( unsigned int id, Shader::ShaderType shaderType, ShaderParameter::Type parameterType );

    // Copy the contents of another buffer into this one.
    // Buffers must be the same size (in bytes).
    virtual void Copy( std::shared_ptr<Buffer> other );

    // Is this an index buffer or an attribute/vertex buffer?
    virtual BufferType GetType() const;
    // How many elements does this buffer contain?
    virtual unsigned int GetElementCount() const;

private:
    Microsoft::WRL::ComPtr<ID3D11Device2> m_pDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext2> m_pDeviceContext;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_pBuffer;

    // The stride of the vertex buffer in bytes.
    UINT m_uiStride;
    // How this buffer should be bound.
    UINT m_BindFlags;
    // The number of elements in this buffer.
    UINT m_uiCount;
    bool m_bIsBound;
};
