#pragma once

#include "Object.h"

class Scene;
class SceneNode;
class Mesh;

class Visitor : public Object
{
public:
    virtual void Visit( Scene& scene ) = 0;
    virtual void Visit( SceneNode& node ) = 0;
    virtual void Visit( Mesh& mesh ) = 0;
};