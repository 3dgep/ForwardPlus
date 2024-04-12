#include <GraphicsTestPCH.h>

#include <Mesh.h>
#include <Material.h>

#include <TransparentPass.h>

TransparentPass::TransparentPass( std::shared_ptr<Scene> scene, std::shared_ptr<PipelineState> pipeline )
    : base( scene, pipeline )
{}

TransparentPass::~TransparentPass()
{}

void TransparentPass::Visit( Mesh& mesh )
{
    std::shared_ptr<Material> pMaterial = mesh.GetMaterial();
    if ( pMaterial && pMaterial->IsTransparent() )
    {
        mesh.Render( GetRenderEventArgs() );
    }
}
