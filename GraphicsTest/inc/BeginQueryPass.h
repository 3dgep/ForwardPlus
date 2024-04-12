#pragma once

#include "AbstractPass.h"

class Query;

// A render pass to begin a GPU query.
class BeginQueryPass : public AbstractPass
{
public:
    BeginQueryPass( std::shared_ptr<Query> query );
    virtual ~BeginQueryPass();

    virtual void Render( RenderEventArgs& e );

private:
    std::shared_ptr<Query> m_pQuery;
};