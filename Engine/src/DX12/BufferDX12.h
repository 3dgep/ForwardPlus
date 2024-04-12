#pragma once

#include <Buffer.h>

class BufferDX12 : public Buffer
{
public:

	BufferDX12( ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, Buffer::BufferType bufferType, const void* data, size_t count, UINT stride );
	~BufferDX12();

	// Is this an index buffer or an attribute/vertex buffer?
	virtual Buffer::BufferType GetType() const;
	// How many elements does this buffer contain?
	virtual unsigned int GetElementCount() const;

    virtual void Copy( std::shared_ptr<Buffer> other );

	// This method is not used for DX12 buffers.
	// Calling this method on a DX12 buffer will throw an exception.
    virtual bool Bind( unsigned int id, Shader::ShaderType shaderType, ShaderParameter::Type parameterType );
    virtual void UnBind( unsigned int id, Shader::ShaderType shaderType, ShaderParameter::Type parameterType );

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource;

	// The stride of the vertex buffer in bytes.
	UINT m_uiStride;
	// How this buffer should be bound.
	Buffer::BufferType m_BufferType;
	// The number of elements in this buffer.
	UINT m_uiCount;

	// The size of the buffer in bytes.
	UINT m_uiBufferSize; 

	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	// Bind the buffer to a particular attribute ID or slot
	// The application should not try to bind a buffer directly.
	// The buffers should be associated to a MeshDX12 object and only 
	// that object is allowed to bind a buffer.
	// TODO: Make the MeshDX12 class a friend of this one so it can bind
	// the buffer to the mesh's command list.
	virtual bool Bind(unsigned int id, ID3D12GraphicsCommandList* pCommandList);
};
