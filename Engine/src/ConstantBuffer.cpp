#include <EnginePCH.h>

#include <ConstantBuffer.h>

Buffer::BufferType ConstantBuffer::GetType() const
{
    return Buffer::ConstantBuffer;
}

unsigned int ConstantBuffer::GetElementCount() const
{
    return 1;
}
