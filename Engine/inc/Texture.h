#pragma once
/**
 * A texture class for loading and using textures in shaders.
 */

#include "Object.h"
#include "Shader.h"
#include "ShaderParameter.h"
#include "ClearFlags.h"
#include "Pixel.h"

class Texture : public Object
{
public:
	typedef Object base;

    enum class Dimension
    {
        Texture1D,
        Texture1DArray,
        Texture2D,
        Texture2DArray,
        Texture3D,
        TextureCube,
    };

    // The number of components used to create the texture.
    enum class Components
    {
        R,              // One red component.
        RG,             // Red, and green components.
        RGB,            // Red, green, and blue components.
        RGBA,           // Red, green, blue, and alpha components.
        Depth,          // Depth component.
        DepthStencil    // Depth and stencil in the same texture.
    };

    // The type of components in the texture.
    enum class Type
    {
        Typeless,           // Typeless formats.
        // TODO: sRGB type
        UnsignedNormalized, // Unsigned normalized (8, 10, or 16-bit unsigned integer values mapped to the range [0..1])
        SignedNormalized,   // Signed normalized (8, or 16-bit signed integer values mapped to the range [-1..1])
        Float,              // Floating point format (16, or 32-bit).
        UnsignedInteger,    // Unsigned integer format (8, 16, or 32-bit unsigned integer formats).
        SignedInteger,      // Signed integer format (8, 16, or 32-bit signed integer formats).
    };

    struct TextureFormat
    {
        Texture::Components Components;
        Texture::Type Type;
        
        // For multi-sample textures, we can specify how many samples we want 
        // to use for this texture. Valid values are usually in the range [1 .. 16]
        // depending on hardware support.
        // A value of 1 will effectively disable multisampling in the texture.
        uint8_t NumSamples;

        // Components should commonly be 8, 16, or 32-bits but some texture formats
        // support 1, 10, 11, 12, or 24-bits per component.
        uint8_t RedBits;
        uint8_t GreenBits;
        uint8_t BlueBits;
        uint8_t AlphaBits;
        uint8_t DepthBits;
        uint8_t StencilBits;

        // By default create a 4-component unsigned normalized texture with 8-bits per component and no multisampling.
        TextureFormat( Texture::Components components = Components::RGBA,
                       Texture::Type type = Type::UnsignedNormalized,
                       uint8_t numSamples = 1,
                       uint8_t redBits = 8,
                       uint8_t greenBits = 8,
                       uint8_t blueBits = 8,
                       uint8_t alphaBits = 8,
                       uint8_t depthBits = 0,
                       uint8_t stencilBits = 0 )
            : Components( components )
            , Type( type )
            , NumSamples( numSamples )
            , RedBits( redBits )
            , GreenBits( greenBits )
            , BlueBits( blueBits )
            , AlphaBits( alphaBits )
            , DepthBits( depthBits )
            , StencilBits( stencilBits )
        {}

        // TODO: Define some commonly used texture formats.
    };

    // For cube maps, we may need to access a particular face of the cube map.
    enum class CubeFace
    {
        Right,  // +X
        Left,   // -X
        Top,    // +Y
        Bottom, // -Y
        Front,  // +Z
        Back,   // -Z
    };

    /**
     * Load a 2D texture from a file path.
     */
    virtual bool LoadTexture2D( const std::wstring& fileName ) = 0;

    /**
     * Load a cubemap texture from a file path.
     */
    virtual bool LoadTextureCube( const std::wstring& fileName ) = 0;

    /**
     * Generate mip maps for a texture.
     * For texture formats that don't support mipmapping,
     * this function does nothing.
     */
    virtual void GenerateMipMaps() = 0;

    /**
     * Get a pointer to a particular face of a cubemap texture.
     * For 1D, and 2D textures, this function always returns the only
     * face of the texture (the texture itself).
     */
    virtual std::shared_ptr<Texture> GetFace( CubeFace face ) const = 0;

    /**
     * 3D textures store several slices of 2D textures.
     * Use this function to get a single 2D slice of a 3D texture.
     * For Cubemaps, this function can be used to get a face of the cubemap.
     * For 1D and 2D textures, this function will always return the texture
     * itself.
     */
    virtual std::shared_ptr<Texture> GetSlice( unsigned int slice ) const = 0;

    // Get the width of the textures in texels.
	virtual uint16_t GetWidth() const = 0;
    // Get the height of the texture in texles.
	virtual uint16_t GetHeight() const = 0;
    // Get the depth of the texture in texture slices for 3D textures, or 
    // cube faces for cubemap textures.
    virtual uint16_t GetDepth() const = 0;

    // Get the bits-per-pixel of the texture.
    virtual uint8_t GetBPP() const = 0;

    // Check to see if this texture has an alpha channel.
    virtual bool IsTransparent() const = 0;

    // Resize the texture to the new dimensions.
    // Resizing a texture will cause the original texture to be discarded.
    // Only use with "dynamic" textures (not ones loaded from a texture file).
    // @param width The width of the texture (for 1D, 2D, and 3D textures or size of a cubemap face for Cubemap textures)
    // @param height The height of the texture (for 2D, 3D textures)
    // @param depth The depth of the texture (for 3D textures only)
    virtual void Resize( uint16_t width, uint16_t height = 0, uint16_t depth = 0 ) = 0;


    /**
     * Plot a color to the texture.
     * This method is only valid for texture created with CPUAccess::Write access.
     * @param coord The non-normalized texture coordinate.
     * @param color The color to plot (RGBA).
     */
    template< typename T >
    void Plot( glm::ivec2 coord, const T& color );

    /**
     * Retrieve the pixel at a particular location in the 
     * texture. 
     * This method is only valid for textures created with CPUAccess::Read access.
     * @param coord The non-normalized texture coordinate.
     * @return The pixel cast to the requested type.
     */
    template< typename T >
    T FetchPixel( glm::ivec2 coord );

    /**
     * Copy the contents of one texture into this one.
     * Textures must both be the same size and have compatible types.
     * @see https://msdn.microsoft.com/en-us/library/windows/desktop/ff476392(v=vs.85).aspx
     * @see https://www.opengl.org/sdk/docs/man/html/glCopyTexSubImage2D.xhtml
     */
    virtual void Copy( std::shared_ptr<Texture> other ) = 0;

    /**
     * Clear the texture.
     * @param color The color to clear the texture to.
     * @param depth The depth value to use for depth textures.
     * @param stencil The stencil value to use for depth/stencil textures.
     */
    virtual void Clear( ClearFlags clearFlags = ClearFlags::All, const glm::vec4& color = glm::vec4(0), float depth = 1.0f, uint8_t stencil = 0 ) = 0;

    /**
     * Bind this texture for use by the shaders.
     */
    virtual void Bind( uint32_t ID, Shader::ShaderType shaderType, ShaderParameter::Type parameterType ) = 0;

    /**
     * Unbind the texture.
     */
    virtual void UnBind( uint32_t ID, Shader::ShaderType shaderType, ShaderParameter::Type parameterType ) = 0;

protected:
    virtual void Plot( glm::ivec2 coord, const uint8_t* pixel, size_t size ) = 0;
    virtual void FetchPixel( glm::ivec2 coord, uint8_t*& pixel, size_t size ) = 0;

private:
};

#include "Texture.inl"