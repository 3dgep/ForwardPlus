#pragma once

#include "BasePass.h"

// A pass that renders the opaque geometry in the scene.
class OpaquePass : public BasePass
{
public:
    typedef BasePass base;

    OpaquePass( std::shared_ptr<Scene> scene, std::shared_ptr<PipelineState> pipeline );
    virtual ~OpaquePass();

    virtual void Visit( Mesh& mesh );

protected:

private:
};