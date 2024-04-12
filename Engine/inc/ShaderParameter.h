#pragma once
#include <Object.h>

class ConstantBuffer;
class Texture;
class SamplerState;
class StructuredBuffer;

class ShaderParameter : public Object
{
public:
    typedef Object base;

    enum class Type
    {
        Invalid,    // Invalid parameter. Doesn't store a type.
        Texture,    // Texture.
        Sampler,    // Texture sampler.
        Buffer,     // Buffers, ConstantBuffers, StructuredBuffers.
        RWTexture,  // Texture that can be written to in a shader (using Store operations).
        RWBuffer,   // Read/write structured buffers.
    };

    template <typename T>
    void Set( std::shared_ptr<T> value );

    template <typename T>
    std::shared_ptr<T> Get() const;

    // Get the type of the stored parameter.
    virtual Type GetType() const = 0;

    // Bind the shader parameter to a specific slot for the given shader type.
    virtual void Bind() = 0;
    virtual void UnBind() = 0;

    // Test to see if this is a valid shader parameter.
    virtual bool IsValid() const;

protected:
    virtual void SetConstantBuffer( std::shared_ptr<ConstantBuffer> constantBuffer ) = 0;
    virtual void SetTexture( std::shared_ptr<Texture> texture ) = 0;
    virtual void SetSampler( std::shared_ptr<SamplerState> sampler ) = 0;
    virtual void SetStructuredBuffer( std::shared_ptr<StructuredBuffer> rwBuffer ) = 0;

private:
};

// Template definitions
#include "ShaderParameter.inl"