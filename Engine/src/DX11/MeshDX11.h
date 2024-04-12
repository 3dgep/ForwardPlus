#pragma once

#include <Mesh.h>

class MeshDX11 : public Mesh
{
public:
	MeshDX11( ID3D11Device2* pDevice );
	virtual ~MeshDX11();

	virtual void AddVertexBuffer( const BufferBinding& binding, std::shared_ptr<Buffer> buffer );
    virtual void SetIndexBuffer( std::shared_ptr<Buffer> buffer );

    virtual void SetMaterial( std::shared_ptr<Material> material );
    virtual std::shared_ptr<Material> GetMaterial() const;

	virtual void Render( RenderEventArgs& renderArgs );

    virtual void Accept( Visitor& visitor );

private:
	typedef std::map<BufferBinding, std::shared_ptr<Buffer> > BufferMap;
	BufferMap m_VertexBuffers;

    std::shared_ptr<Buffer> m_pIndexBuffer;
    std::shared_ptr<Material> m_pMaterial;

	Microsoft::WRL::ComPtr<ID3D11Device2> m_pDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext2> m_pDeviceContext;
};