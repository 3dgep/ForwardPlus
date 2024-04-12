#pragma once

#include <Mesh.h>

class Buffer;

class MeshDX12 : public Mesh
{
public:
	MeshDX12( ID3D12Device* pDevice );
	virtual ~MeshDX12();

	virtual void AddVertexBuffer( const BufferBinding& semantic, const Buffer* buffer );
    virtual void SetIndexBuffer( const Buffer* buffer );

    virtual void SetMaterial( const Material* material );

	virtual void Render( RenderEventArgs& renderEventArgs );

private:
	typedef std::map<BufferBinding, Buffer*> BufferMap;
	BufferMap m_VertexBuffers;

    Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
    
    Buffer* m_pIndexBuffer;
    const Material* m_pMaterial;

};