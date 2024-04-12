#pragma once

#include "AbstractPass.h"

class Buffer;

/**
 * A render pass that copies a GPU buffer to another.
 * Both buffers must be the same size (in bytes) and their internal 
 * formats should be the same.
 */
class CopyBufferPass : public AbstractPass
{
public:
    CopyBufferPass( std::shared_ptr<Buffer> destinationBuffer,
                     std::shared_ptr<Buffer> sourceBuffer );
    virtual ~CopyBufferPass();

    virtual void Render( RenderEventArgs& e );

private:
    std::shared_ptr<Buffer> m_SourceBuffer;
    std::shared_ptr<Buffer> m_DestinationBuffer;
};
