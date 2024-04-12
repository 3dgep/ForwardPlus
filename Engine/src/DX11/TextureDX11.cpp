#include <EnginePCH.h>
#include <Timer.h>
#include "TextureDX11.h"

static void ReportAndThrowTextureFormatError( const Texture::TextureFormat& format, const std::string& file, int line, const std::string& function, const std::string& message )
{
    std::stringstream ss;
    ss << message << std::endl;
    ss << "Components: ";
    switch ( format.Components )
    {
    case Texture::Components::R:
        ss << "R" << std::endl;
        break;
    case Texture::Components::RG:
        ss << "RG" << std::endl;
        break;
    case Texture::Components::RGB:
        ss << "RGB" << std::endl;
        break;
    case Texture::Components::RGBA:
        ss << "RGBA" << std::endl;
        break;
    case Texture::Components::Depth:
        ss << "Depth" << std::endl;
        break;
    case Texture::Components::DepthStencil:
        ss << "DepthStencil" << std::endl;
        break;
    default:
        ss << "Unknown" << std::endl;
        break;
    }

    ss << "Type:";
    switch ( format.Type )
    {
    case Texture::Type::Typeless:
        ss << "Typeless" << std::endl;
        break;
    case Texture::Type::UnsignedNormalized:
        ss << "UnsignedNormalized" << std::endl;
        break;
    case Texture::Type::SignedNormalized:
        ss << "SignedNormalized" << std::endl;
        break;
    case Texture::Type::Float:
        ss << "Float" << std::endl;
        break;
    case Texture::Type::UnsignedInteger:
        ss << "UnsignedInteger" << std::endl;
        break;
    case Texture::Type::SignedInteger:
        ss << "SignedInteger" << std::endl;
        break;
    default:
        ss << "Unknown" << std::endl;
        break;
    }

    ss << "RedBits:     " << (int32_t)format.RedBits << std::endl;
    ss << "GreenBits:   " << (int32_t)format.GreenBits << std::endl;
    ss << "BlueBits:    " << (int32_t)format.BlueBits << std::endl;
    ss << "AlphaBits:   " << (int32_t)format.AlphaBits << std::endl;
    ss << "DepthBits:   " << (int32_t)format.DepthBits << std::endl;
    ss << "StencilBits: " << (int32_t)format.StencilBits << std::endl;
    ss << "Num Samples: " << (int32_t)format.NumSamples << std::endl;
 
    ReportErrorAndThrow( file, line, function, ss.str() );
}

#define ReportTextureFormatError( fmt, msg ) ReportAndThrowTextureFormatError( (fmt), __FILE__, __LINE__, __FUNCTION__, (msg) )

TextureDX11::TextureDX11( ID3D11Device2* pDevice )
    : m_pDevice( pDevice )
    , m_TextureWidth( 0 )
    , m_TextureHeight( 0 )
    , m_NumSlices( 0 )
    , m_TextureResourceFormatSupport( 0 )
    , m_DepthStencilViewFormatSupport( 0 )
    , m_ShaderResourceViewFormatSupport( 0 )
    , m_RenderTargetViewFormatSupport( 0 )
    , m_CPUAccess( CPUAccess::None )
    , m_bDynamic( false )
    , m_bUAV( false )
    , m_TextureResourceFormat( DXGI_FORMAT_UNKNOWN )
    , m_DepthStencilViewFormat( DXGI_FORMAT_UNKNOWN )
    , m_ShaderResourceViewFormat( DXGI_FORMAT_UNKNOWN )
    , m_RenderTargetViewFormat( DXGI_FORMAT_UNKNOWN )
    , m_bGenerateMipmaps( false )
    , m_BPP( 0 )
    , m_Pitch( 0 )
    , m_bIsTransparent( false )
    , m_bFileChanged( false )
    , m_bIsDirty( false )
{
    m_pDevice->GetImmediateContext2( &m_pDeviceContext );

    m_Connections.push_back( m_DependencyTracker.FileChanged += boost::bind( &TextureDX11::OnFileChanged, this, _1 ) );
}

// 1D Texture
TextureDX11::TextureDX11( ID3D11Device2* pDevice, uint16_t width, uint16_t slices, const TextureFormat& format, CPUAccess cpuAccess, bool bUAV )
    : m_pDevice( pDevice )
    , m_TextureWidth( 0 )
    , m_TextureHeight( 1 )
    , m_TextureResourceFormatSupport( 0 )
    , m_CPUAccess( cpuAccess )
    , m_bGenerateMipmaps( false )
    , m_BPP( 0 )
    , m_Pitch( 0 )
    , m_bIsTransparent( false )
    , m_bFileChanged( false )
    , m_bIsDirty( false )
{
    m_pDevice->GetImmediateContext2( &m_pDeviceContext );

    m_Connections.push_back( m_DependencyTracker.FileChanged += boost::bind( &TextureDX11::OnFileChanged, this, _1 ) );

    m_NumSlices = glm::max<uint16_t>( slices, 1 );

    m_TextureDimension = Dimension::Texture1D;
    if ( m_NumSlices > 1 )
    {
        m_TextureDimension = Dimension::Texture1DArray;
    }

    // Translate to DXGI format.
    DXGI_FORMAT dxgiFormat = TranslateFormat( format );
    m_SampleDesc = GetSupportedSampleCount( dxgiFormat, format.NumSamples );

    // Translate back to original format (for best match format)
    m_TextureFormat = TranslateFormat( dxgiFormat, format.NumSamples );

    // Convert Depth/Stencil formats to typeless texture resource formats
    m_TextureResourceFormat = GetTextureFormat( dxgiFormat );
    // Convert typeless formats to Depth/Stencil view formats
    m_DepthStencilViewFormat = GetDSVFormat( dxgiFormat );
    // Convert depth/stencil and typeless formats to Shader Resource View formats
    m_ShaderResourceViewFormat = GetSRVFormat( dxgiFormat );
    // Convert typeless formats to Render Target View formats
    m_RenderTargetViewFormat = GetRTVFormat( dxgiFormat );
    // Convert typeless format to Unordered Access View formats.
    m_UnorderedAccessViewFormat = GetUAVFormat( dxgiFormat );

    m_BPP = GetBPP( m_TextureResourceFormat );

    // Query for texture format support.
    if ( FAILED( m_pDevice->CheckFormatSupport( m_TextureResourceFormat, &m_TextureResourceFormatSupport ) ) )
    {
        ReportError( "Failed to query texture resource format support." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_DepthStencilViewFormat, &m_DepthStencilViewFormatSupport ) ) )
    {
        ReportError( "Failed to query depth/stencil view format support." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_ShaderResourceViewFormat, &m_ShaderResourceViewFormatSupport ) ) )
    {
        ReportError( "Failed to query shader resource view format support." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_RenderTargetViewFormat, &m_RenderTargetViewFormatSupport ) ) )
    {
        ReportError( "Failed to query render target view format support." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_UnorderedAccessViewFormat, &m_UnorderedAccessViewFormatSupport ) ) )
    {
        ReportError( "Failed to query unordered access view format support." );
    }
    if ( ( m_TextureResourceFormatSupport & D3D11_FORMAT_SUPPORT_TEXTURE1D ) == 0 )
    {
        ReportTextureFormatError( m_TextureFormat, "Unsupported texture format for 1D textures." );
    }
    // Can the texture be dynamically modified on the CPU?
    m_bDynamic = (int)m_CPUAccess != 0 && ( m_TextureResourceFormatSupport & D3D11_FORMAT_SUPPORT_CPU_LOCKABLE ) != 0;
    // Can mipmaps be automatically generated for this texture format?
    m_bGenerateMipmaps = !m_bDynamic && ( m_ShaderResourceViewFormatSupport & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN ) != 0;
    // Are UAVs supported?
    m_bUAV = bUAV && ( m_UnorderedAccessViewFormatSupport & D3D11_FORMAT_SUPPORT_SHADER_LOAD ) != 0;

    // Resize the texture to the requested dimension.
    Resize( width );
}

// 2D Texture
TextureDX11::TextureDX11( ID3D11Device2* pDevice, uint16_t width, uint16_t height, uint16_t slices, const TextureFormat& format, CPUAccess cpuAccess, bool bUAV )
    : m_pDevice( pDevice )
    , m_pTexture2D( nullptr )
    , m_pShaderResourceView( nullptr )
    , m_pRenderTargetView( nullptr )
    , m_TextureWidth( 0 )
    , m_TextureHeight( 0 )
    , m_BPP( 0 )
    , m_TextureFormat( format )
    , m_CPUAccess( cpuAccess )
    , m_bGenerateMipmaps( false )
    , m_bIsTransparent( true )
    , m_bFileChanged( false )
    , m_bIsDirty( false )
{
    m_pDevice->GetImmediateContext2( &m_pDeviceContext );

    m_Connections.push_back( m_DependencyTracker.FileChanged += boost::bind( &TextureDX11::OnFileChanged, this, _1 ) );

    m_NumSlices = glm::max<uint16_t>( slices, 1 );

    m_TextureDimension = Dimension::Texture2D;
    if ( m_NumSlices > 1 )
    {
        m_TextureDimension = Dimension::Texture2DArray;
    }

    // Translate to DXGI format.
    DXGI_FORMAT dxgiFormat = TranslateFormat( format );
    m_SampleDesc = GetSupportedSampleCount( dxgiFormat, format.NumSamples );

    // Translate back to original format (for best match format).
    m_TextureFormat = TranslateFormat( dxgiFormat, format.NumSamples );

    // Convert Depth/Stencil formats to typeless texture resource formats.
    m_TextureResourceFormat = GetTextureFormat( dxgiFormat );
    // Convert typeless formats to Depth/Stencil view formats.
    m_DepthStencilViewFormat = GetDSVFormat( dxgiFormat );
    // Convert depth/stencil and typeless formats to Shader Resource View formats.
    m_ShaderResourceViewFormat = GetSRVFormat( dxgiFormat );
    // Convert typeless formats to Render Target View formats.
    m_RenderTargetViewFormat = GetRTVFormat( dxgiFormat );
    // Convert typeless format to Unordered Access View formats.
    m_UnorderedAccessViewFormat = GetUAVFormat( dxgiFormat );

    m_BPP = GetBPP( m_TextureResourceFormat );

    // Query for texture format support.
    if ( FAILED( m_pDevice->CheckFormatSupport( m_TextureResourceFormat, &m_TextureResourceFormatSupport ) ) )
    {
        ReportError( "Failed to query texture resource format support." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_DepthStencilViewFormat, &m_DepthStencilViewFormatSupport ) ) )
    {
        ReportError( "Failed to query depth/stencil format support." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_ShaderResourceViewFormat, &m_ShaderResourceViewFormatSupport ) ) )
    {
        ReportError( "Failed to query shader resource format support." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_RenderTargetViewFormat, &m_RenderTargetViewFormatSupport ) ) )
    {
        ReportError( "Failed to query render target format support." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_UnorderedAccessViewFormat, &m_UnorderedAccessViewFormatSupport) ) )
    {
        ReportError( "Failed to query render target format support." );
    }
    if ( ( m_TextureResourceFormatSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D ) == 0 )
    {
        ReportTextureFormatError( m_TextureFormat, "Unsupported texture format for 2D textures." );
    }
    // Can the texture be dynamically modified on the CPU?
    m_bDynamic = (int)m_CPUAccess != 0 && ( m_TextureResourceFormatSupport & D3D11_FORMAT_SUPPORT_CPU_LOCKABLE ) != 0;
    // Can mipmaps be automatically generated for this texture format?
    m_bGenerateMipmaps = !m_bDynamic && ( m_ShaderResourceViewFormatSupport & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN ) != 0;
    // Are UAVs supported?
    m_bUAV = bUAV && ( m_UnorderedAccessViewFormatSupport & D3D11_FORMAT_SUPPORT_SHADER_LOAD ) != 0;

    Resize( width, height );
}

// 3D Texture
TextureDX11::TextureDX11( Tex3DCtor, ID3D11Device2* pDevice, uint16_t width, uint16_t height, uint16_t depth, const TextureFormat& format, CPUAccess cpuAccess, bool bUAV )
    : m_bFileChanged( false )
{
    m_pDevice->GetImmediateContext2( &m_pDeviceContext );

    m_Connections.push_back( m_DependencyTracker.FileChanged += boost::bind( &TextureDX11::OnFileChanged, this, _1 ) );

    m_NumSlices = glm::max<uint16_t>( depth, 1 );

    m_TextureDimension = Dimension::Texture3D;

    // Translate to DXGI format.
    DXGI_FORMAT dxgiFormat = TranslateFormat( format );
    m_SampleDesc = GetSupportedSampleCount( dxgiFormat, format.NumSamples );

    // Translate back to original format (for best match format)
    m_TextureFormat = TranslateFormat( dxgiFormat, format.NumSamples );

    // Convert Depth/Stencil formats to typeless texture resource formats
    m_TextureResourceFormat = GetTextureFormat( dxgiFormat );
    // Convert typeless formats to Depth/Stencil view formats
    m_DepthStencilViewFormat = GetDSVFormat( dxgiFormat );
    // Convert depth/stencil and typeless formats to Shader Resource View formats
    m_ShaderResourceViewFormat = GetSRVFormat( dxgiFormat );
    // Convert typeless formats to Render Target View formats
    m_RenderTargetViewFormat = GetRTVFormat( dxgiFormat );
    // Convert typeless format to Unordered Access View formats.
    m_UnorderedAccessViewFormat = GetUAVFormat( dxgiFormat );

    m_BPP = GetBPP( m_TextureResourceFormat );

    // Query for texture format support.
    if ( FAILED( m_pDevice->CheckFormatSupport( m_TextureResourceFormat, &m_TextureResourceFormatSupport ) ) )
    {
        ReportError( "Failed to query texture resource format support." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_DepthStencilViewFormat, &m_DepthStencilViewFormatSupport ) ) )
    {
        ReportError( "Failed to query depth/stencil format support." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_ShaderResourceViewFormat, &m_ShaderResourceViewFormatSupport ) ) )
    {
        ReportError( "Failed to query shader resource format support." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_RenderTargetViewFormat, &m_RenderTargetViewFormatSupport ) ) )
    {
        ReportError( "Failed to query render target format support." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_UnorderedAccessViewFormat, &m_UnorderedAccessViewFormatSupport ) ) )
    {
        ReportError( "Failed to query render target format support." );
    }

    if ( ( m_TextureResourceFormatSupport & D3D11_FORMAT_SUPPORT_TEXTURE3D ) == 0 )
    {
        ReportTextureFormatError( m_TextureFormat, "Unsupported texture format for 3D textures." );
    }
    // Can the texture be dynamically modified on the CPU?
    m_bDynamic = (int)m_CPUAccess != 0 && ( m_TextureResourceFormatSupport & D3D11_FORMAT_SUPPORT_CPU_LOCKABLE ) != 0;
    // Can mipmaps be automatically generated for this texture format?
    m_bGenerateMipmaps = !m_bDynamic && ( m_ShaderResourceViewFormatSupport & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN ) != 0;
    // Are UAVs supported?
    m_bUAV = bUAV && ( m_UnorderedAccessViewFormatSupport & D3D11_FORMAT_SUPPORT_SHADER_LOAD ) != 0;

}

// CUBE Texture
TextureDX11::TextureDX11( CubeCtor, ID3D11Device2* pDevice, uint16_t size, uint16_t count, const TextureFormat& format, CPUAccess cpuAccess, bool bUAV )
    : m_bFileChanged( false )
{
    m_Connections.push_back( m_DependencyTracker.FileChanged += boost::bind( &TextureDX11::OnFileChanged, this, _1 ) );

    m_TextureDimension = Texture::Dimension::TextureCube;

    m_TextureWidth = m_TextureHeight = size;

    // Translate to DXGI format.
    DXGI_FORMAT dxgiFormat = TranslateFormat( format );
    m_SampleDesc = GetSupportedSampleCount( dxgiFormat, format.NumSamples );

    // Translate back to original format (for best match format)
    m_TextureFormat = TranslateFormat( dxgiFormat, format.NumSamples );

    // Convert Depth/Stencil formats to typeless texture resource formats
    m_TextureResourceFormat = GetTextureFormat( dxgiFormat );
    // Convert typeless formats to Depth/Stencil view formats
    m_DepthStencilViewFormat = GetDSVFormat( dxgiFormat );
    // Convert depth/stencil and typeless formats to Shader Resource View formats
    m_ShaderResourceViewFormat = GetSRVFormat( dxgiFormat );
    // Convert typeless formats to Render Target View formats
    m_RenderTargetViewFormat = GetRTVFormat( dxgiFormat );
    // Convert typeless format to Unordered Access View formats.
    m_UnorderedAccessViewFormat = GetUAVFormat( dxgiFormat );

    // Query for texture format support.
    if ( FAILED( m_pDevice->CheckFormatSupport( m_TextureResourceFormat, &m_TextureResourceFormatSupport ) ) )
    {
        ReportError( "Failed to query texture resource format support." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_DepthStencilViewFormat, &m_DepthStencilViewFormatSupport ) ) )
    {
        ReportError( "Failed to query depth/stencil format support." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_ShaderResourceViewFormat, &m_ShaderResourceViewFormatSupport ) ) )
    {
        ReportError( "Failed to query shader resource format support." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_RenderTargetViewFormat, &m_RenderTargetViewFormatSupport ) ) )
    {
        ReportError( "Failed to query render target format support." );
    }
    if ( ( m_TextureResourceFormatSupport & D3D11_FORMAT_SUPPORT_TEXTURECUBE ) == 0 )
    {
        ReportTextureFormatError( m_TextureFormat, "Unsupported texture format for cubemap textures." );
    }
    if ( FAILED( m_pDevice->CheckFormatSupport( m_UnorderedAccessViewFormat, &m_UnorderedAccessViewFormatSupport ) ) )
    {
        ReportError( "Failed to query render target format support." );
    }

    if ( ( m_TextureResourceFormatSupport & D3D11_FORMAT_SUPPORT_TEXTURECUBE ) == 0 )
    {
        ReportTextureFormatError( m_TextureFormat, "Unsupported texture format for cube textures." );
    }

    // Can the texture be dynamically modified on the CPU?
    m_bDynamic = ( (int)m_CPUAccess & (int)CPUAccess::Write ) != 0 && ( m_TextureResourceFormatSupport & D3D11_FORMAT_SUPPORT_CPU_LOCKABLE ) != 0;
    // Can mipmaps be automatically generated for this texture format?
    m_bGenerateMipmaps = !m_bDynamic && ( m_ShaderResourceViewFormatSupport & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN ) != 0; // && ( m_RenderTargetViewFormatSupport & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN ) != 0;
    // Are UAVs supported?
    m_bUAV = bUAV && ( m_UnorderedAccessViewFormatSupport & D3D11_FORMAT_SUPPORT_SHADER_LOAD ) != 0;

}

TextureDX11::~TextureDX11()
{}

void PrintMetaData( FREE_IMAGE_MDMODEL model, FIBITMAP* dib )
{
    FITAG* metadataTag;
    FIMETADATA* metadata = FreeImage_FindFirstMetadata( model, dib, &metadataTag );
    if ( metadata )
    {
        std::stringstream ss;
        do
        {
            const char* key = FreeImage_GetTagKey( metadataTag );
            const char* value = FreeImage_TagToString( model, metadataTag );
            const char* description = FreeImage_GetTagDescription( metadataTag );

            ss << "key[" << key << "] = " << value << "; Description: ";

            if ( description )
            {
                ss << description << std::endl;
            }
            else
            {
                ss << "(none)" << std::endl;
            }

        } while ( FreeImage_FindNextMetadata( metadata, &metadataTag ) );
        FreeImage_FindCloseMetadata( metadata );

        OutputDebugStringA( ss.str().c_str() );
    }
}

bool TextureDX11::LoadTexture2D( const std::wstring& fileName )
{
    fs::path filePath( fileName );
    if ( !fs::exists( filePath ) || !fs::is_regular_file( filePath ) )
    {
        ReportError( "Could not load texture: " + filePath.string() );
        return false;
    }

    m_TextureFileName = fileName;
    m_DependencyTracker = DependencyTracker( fileName );
    // Try to load the dependency file for the texture asset.
    if ( !m_DependencyTracker.Load() )
    {
        // If loading failed, likely, the dependency tracker file
        // does not exist. Save the default dependency tracker.
        m_DependencyTracker.Save();
    }

    m_DependencyTracker.SetLastLoadTime();

    // Try to determine the file type from the image file.
    FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeU( filePath.c_str() );
    if ( fif == FIF_UNKNOWN )
    {
        fif = FreeImage_GetFIFFromFilenameU( filePath.c_str() );
    }

    if ( fif == FIF_UNKNOWN || !FreeImage_FIFSupportsReading( fif ) )
    {
        ReportError( "Unknow file format: " + filePath.string() );
        return false;
    }

    FIBITMAP* dib = FreeImage_LoadU( fif, filePath.c_str() );
    if ( dib == nullptr || FreeImage_HasPixels( dib ) == FALSE )
    {
        ReportError( "Failed to load image: " + filePath.string() );
        return false;
    }

    //// Check to see if we need to flip the image
    //for ( int model = 0; model < FIMD_EXIF_RAW + 1; model++ )
    //{
    //    PrintMetaData( (FREE_IMAGE_MDMODEL)model, dib );
    //}

    m_BPP = FreeImage_GetBPP( dib );
    FREE_IMAGE_TYPE imageType = FreeImage_GetImageType( dib );

    // Check to see if the texture has an alpha channel.
    m_bIsTransparent = ( FreeImage_IsTransparent( dib ) == TRUE );

    switch ( m_BPP )
    {
    case 8:
    {
        switch ( imageType )
        {
        case FIT_BITMAP:
        {
            m_TextureResourceFormat = DXGI_FORMAT_R8_UNORM;
        }
        break;
        default:
        {
            ReportError( "Unknown image format." );
        }
        break;
        }
    }
    break;
    case 16:
    {
        switch ( imageType )
        {
        case FIT_BITMAP:
        {
            m_TextureResourceFormat = DXGI_FORMAT_R8G8_UNORM;
        }
        break;
        case FIT_UINT16:
        {
            m_TextureResourceFormat = DXGI_FORMAT_R16_UINT;
        }
        break;
        case FIT_INT16:
        {
            m_TextureResourceFormat = DXGI_FORMAT_R16_SINT;
        }
        break;
        default:
        {
            ReportError( "Unknown image format." );
        }
        break;
        }
    }
    break;
    case 32:
    {
        switch ( imageType )
        {
        case FIT_BITMAP:
        {
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
            m_TextureResourceFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
#else
            m_TextureResourceFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
#endif
        }
        break;
        case FIT_FLOAT:
        {
            m_TextureResourceFormat = DXGI_FORMAT_R32_FLOAT;
        }
        break;
        case FIT_INT32:
        {
            m_TextureResourceFormat = DXGI_FORMAT_R32_SINT;
        }
        break;
        case FIT_UINT32:
        {
            m_TextureResourceFormat = DXGI_FORMAT_R32_UINT;
        }
        break;
        default:
        {
            ReportError( "Unknown image format." );
        }
        break;
        }
    }
    break;
    default:
    {
        FIBITMAP* dib32 = FreeImage_ConvertTo32Bits( dib );

        // Unload the original image.
        FreeImage_Unload( dib );

        dib = dib32;

        // Update pixel bit depth (should be 32 now if it wasn't before).
        m_BPP = FreeImage_GetBPP( dib );

#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
        m_TextureResourceFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
#else
        m_TextureResourceFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
#endif
    }
    break;
    }

    m_TextureDimension = Texture::Dimension::Texture2D;
    m_TextureWidth = FreeImage_GetWidth( dib );
    m_TextureHeight = FreeImage_GetHeight( dib );
    m_NumSlices = 1;
    m_Pitch = FreeImage_GetPitch( dib );

    m_ShaderResourceViewFormat = m_RenderTargetViewFormat = m_TextureResourceFormat;
    m_SampleDesc = GetSupportedSampleCount( m_TextureResourceFormat, 1 );

    if ( FAILED( m_pDevice->CheckFormatSupport( m_TextureResourceFormat, &m_TextureResourceFormatSupport ) ) )
    {
        ReportError( "Failed to query format support." );
    }
    if ( ( m_TextureResourceFormatSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D ) == 0 )
    {
        ReportTextureFormatError( m_TextureFormat, "Unsupported texture format for 2D textures." );
        return false;
    }

    m_ShaderResourceViewFormatSupport = m_RenderTargetViewFormatSupport = m_TextureResourceFormatSupport;

    // Can mipmaps be automatically generated for this texture format?
    m_bGenerateMipmaps = !m_bDynamic && ( m_ShaderResourceViewFormatSupport & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN ) != 0;

    // Load the texture data into a GPU texture.
    D3D11_TEXTURE2D_DESC textureDesc = { 0 };

    textureDesc.Width = m_TextureWidth;
    textureDesc.Height = m_TextureHeight;
    textureDesc.MipLevels = m_bGenerateMipmaps ? 0 : 1;
    textureDesc.ArraySize = m_NumSlices;
    textureDesc.Format = m_TextureResourceFormat;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    if ( ( m_ShaderResourceViewFormatSupport & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE ) != 0 )
    {
        textureDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    }
    if ( ( m_RenderTargetViewFormatSupport & D3D11_FORMAT_SUPPORT_RENDER_TARGET ) != 0 )
    {
        textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
    }
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = m_bGenerateMipmaps ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

    BYTE* textureData = FreeImage_GetBits( dib );

    D3D11_SUBRESOURCE_DATA subresourceData;
    subresourceData.pSysMem = textureData;
    subresourceData.SysMemPitch = m_Pitch;
    subresourceData.SysMemSlicePitch = 0;

    if ( FAILED( m_pDevice->CreateTexture2D( &textureDesc, m_bGenerateMipmaps ? nullptr : &subresourceData, &m_pTexture2D ) ) )
    {
        ReportError( "Failed to create texture." );
        return false;
    }

    // Create a Shader resource view for the texture.
    D3D11_SHADER_RESOURCE_VIEW_DESC resourceViewDesc;

    resourceViewDesc.Format = m_ShaderResourceViewFormat;
    resourceViewDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
    resourceViewDesc.Texture2D.MipLevels = m_bGenerateMipmaps ? -1 : 1;
    resourceViewDesc.Texture2D.MostDetailedMip = 0;

    if ( FAILED( m_pDevice->CreateShaderResourceView( m_pTexture2D.Get(), &resourceViewDesc, &m_pShaderResourceView ) ) )
    {
        ReportError( "Failed to create texture resource view." );
        return false;
    }

    // From DirectXTK (28/05/2015) @see https://directxtk.codeplex.com/
    if ( m_bGenerateMipmaps )
    {
        m_pDeviceContext->UpdateSubresource( m_pTexture2D.Get(), 0, nullptr, textureData, m_Pitch, 0 );
        m_pDeviceContext->GenerateMips( m_pShaderResourceView.Get() );
    }

    m_bIsDirty = false;

    // Unload the texture (it should now be on the GPU anyways).
    FreeImage_Unload( dib );

    return true;
}

bool TextureDX11::LoadTextureCube( const std::wstring& fileName )
{
    fs::path filePath( fileName );
    if ( !fs::exists( filePath ) || !fs::is_regular_file( filePath ) )
    {
        ReportError( "Could not load texture: " + filePath.string() );
        return false;
    }

    m_TextureFileName = fileName;
    m_DependencyTracker = DependencyTracker( fileName );
    // Try to load the dependency file for the texture asset.
    if ( !m_DependencyTracker.Load() )
    {
        // If loading failed, likely, the dependency tracker file
        // does not exist. Save the default dependency tracker.
        m_DependencyTracker.Save();
    }

    m_DependencyTracker.SetLastLoadTime();

    // Try to determine the file type from the image file.
    FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeU( filePath.c_str() );
    if ( fif == FIF_UNKNOWN )
    {
        fif = FreeImage_GetFIFFromFilenameU( filePath.c_str() );
    }

    if ( fif == FIF_UNKNOWN || !FreeImage_FIFSupportsReading( fif ) )
    {
        ReportError( "Unknow file format: " + filePath.string() );
        return false;
    }

    FIMULTIBITMAP* dib = FreeImage_OpenMultiBitmap( fif, filePath.string().c_str(), FALSE, TRUE, TRUE );
    if ( dib == nullptr || FreeImage_GetPageCount( dib ) == 0 )
    {
        ReportError( "Failed to load image: " + filePath.string() );
        return false;
    }

    int pageCount = FreeImage_GetPageCount( dib );

    // TODO: DDS cubemap loading with FreeImage?

    return false;
}

void TextureDX11::GenerateMipMaps()
{
    if ( m_bGenerateMipmaps && m_pShaderResourceView )
    {
        m_pDeviceContext->GenerateMips( m_pShaderResourceView.Get() );
    }
}

std::shared_ptr<Texture> TextureDX11::GetFace( CubeFace face ) const
{
    return std::static_pointer_cast<Texture>( std::const_pointer_cast<TextureDX11>( shared_from_this() ) );
}

std::shared_ptr<Texture> TextureDX11::GetSlice( unsigned int slice ) const
{
    return std::static_pointer_cast<Texture>( std::const_pointer_cast<TextureDX11>( shared_from_this() ) );
}

uint16_t TextureDX11::GetWidth() const
{
    return m_TextureWidth;
}

uint16_t TextureDX11::GetHeight() const
{
    return m_TextureHeight;
}

uint16_t TextureDX11::GetDepth() const
{
    return m_NumSlices;
}

uint8_t TextureDX11::GetBPP() const
{
    return m_BPP;
}

bool TextureDX11::IsTransparent() const
{
    return m_bIsTransparent;
}

void TextureDX11::Resize1D( uint16_t width )
{
    if ( m_TextureWidth != width )
    {
        m_pTexture1D.Reset();
        m_pShaderResourceView.Reset();
        m_pRenderTargetView.Reset();
        m_pDepthStencilView.Reset();
        m_pUnorderedAccessView.Reset();

        m_TextureWidth = glm::max<uint16_t>( width, 1 );

        D3D11_TEXTURE1D_DESC textureDesc = { 0 };

        textureDesc.Width = m_TextureWidth;
        textureDesc.ArraySize = m_NumSlices;
        textureDesc.Format = m_TextureResourceFormat;
        textureDesc.MipLevels = 1;

        if ( ( (int)m_CPUAccess & (int)CPUAccess::Read ) != 0 )
        {
            textureDesc.Usage = D3D11_USAGE_STAGING;
            textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
        }
        else if ( ( (int)m_CPUAccess & (int)CPUAccess::Write ) != 0 )
        {
            textureDesc.Usage = D3D11_USAGE_DYNAMIC;
            textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        }
        else
        {
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.CPUAccessFlags = 0;
        }

        if ( !m_bUAV && !m_bDynamic && ( m_DepthStencilViewFormatSupport & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL ) != 0 )
        {
            textureDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        }
        if ( !m_bDynamic && ( m_RenderTargetViewFormatSupport & D3D11_FORMAT_SUPPORT_RENDER_TARGET ) != 0 )
        {
            textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        }
        if ( ( (int)m_CPUAccess & (int)CPUAccess::Read ) == 0 && ( m_ShaderResourceViewFormatSupport & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE ) != 0 )
        {
            textureDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
        if ( m_bUAV )
        {
            textureDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        textureDesc.MiscFlags = m_bGenerateMipmaps ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

        if ( FAILED( m_pDevice->CreateTexture1D( &textureDesc, nullptr, &m_pTexture1D ) ) )
        {
            ReportError( "Failed to create texture." );
            return;
        }

        if ( ( textureDesc.BindFlags &  D3D11_BIND_DEPTH_STENCIL ) != 0 )
        {
            // Create the depth/stencil view for the texture.
            D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
            depthStencilViewDesc.Format = m_DepthStencilViewFormat;
            depthStencilViewDesc.Flags = 0;

            if ( m_NumSlices > 1 )
            {
                depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
                depthStencilViewDesc.Texture1DArray.MipSlice = 0;
                depthStencilViewDesc.Texture1DArray.FirstArraySlice = 0;
                depthStencilViewDesc.Texture1DArray.ArraySize = m_NumSlices;
            }
            else
            {
                depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
                depthStencilViewDesc.Texture1D.MipSlice = 0;
            }

            if ( FAILED( m_pDevice->CreateDepthStencilView( m_pTexture1D.Get(), &depthStencilViewDesc, &m_pDepthStencilView ) ) )
            {
                ReportError( "Failed to create depth/stencil view." );
            }
        }

        if ( ( textureDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE ) != 0 )
        {
            // Create a Shader resource view for the texture.
            D3D11_SHADER_RESOURCE_VIEW_DESC resourceViewDesc;
            resourceViewDesc.Format = m_ShaderResourceViewFormat;

            if ( m_NumSlices > 1 )
            {
                resourceViewDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE1DARRAY;
                resourceViewDesc.Texture1DArray.MipLevels = m_bGenerateMipmaps ? -1 : 1;
                resourceViewDesc.Texture1DArray.MostDetailedMip = 0;
                resourceViewDesc.Texture1DArray.FirstArraySlice = 0;
                resourceViewDesc.Texture1DArray.ArraySize = m_NumSlices;
            }
            else
            {
                resourceViewDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE1D;
                resourceViewDesc.Texture1D.MipLevels = m_bGenerateMipmaps ? -1 : 1;
                resourceViewDesc.Texture1D.MostDetailedMip = 0;
            }

            if ( FAILED( m_pDevice->CreateShaderResourceView( m_pTexture1D.Get(), &resourceViewDesc, &m_pShaderResourceView ) ) )
            {
                ReportError( "Failed to create shader resource view." );
            } 
            else if ( m_bGenerateMipmaps )
            {
                m_pDeviceContext->GenerateMips( m_pShaderResourceView.Get() );
            }
        }

        if ( ( textureDesc.BindFlags & D3D11_BIND_RENDER_TARGET ) != 0 )
        {
            // Create the render target view for the texture.
            D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
            renderTargetViewDesc.Format = m_RenderTargetViewFormat;

            if ( m_NumSlices > 1 )
            {
                renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
                renderTargetViewDesc.Texture1DArray.MipSlice = 0;
                renderTargetViewDesc.Texture1DArray.FirstArraySlice = 0;
                renderTargetViewDesc.Texture1DArray.ArraySize = m_NumSlices;
            }
            else
            {
                renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
                renderTargetViewDesc.Texture1D.MipSlice = 0;
            }

            if ( FAILED( m_pDevice->CreateRenderTargetView( m_pTexture1D.Get(), &renderTargetViewDesc, &m_pRenderTargetView ) ) )
            {
                ReportError( "Failed to create render target view." );
            }
        }
        if ( ( textureDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS ) != 0 )
        {
            // UAVs cannot be multi-sampled.
            assert( m_SampleDesc.Count == 1 );

            // Create a Shader resource view for the texture.
            D3D11_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc;
            unorderedAccessViewDesc.Format = m_UnorderedAccessViewFormat;

            if ( m_NumSlices > 1 )
            {
                unorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
                unorderedAccessViewDesc.Texture1DArray.FirstArraySlice = 0;
                unorderedAccessViewDesc.Texture1DArray.ArraySize = m_NumSlices;
                unorderedAccessViewDesc.Texture1DArray.MipSlice = 0;
            }
            else
            {
                unorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
                unorderedAccessViewDesc.Texture2D.MipSlice = 0;
            }

            if ( FAILED( m_pDevice->CreateUnorderedAccessView( m_pTexture1D.Get(), &unorderedAccessViewDesc, &m_pUnorderedAccessView ) ) )
            {
                ReportError( "Failed to create unordered access view." );
            }
        }

        assert( m_BPP > 0 && m_BPP % 8 == 0 );
        m_Buffer.resize( width * ( m_BPP / 8 ) );
    }
}

void TextureDX11::Resize2D( uint16_t width, uint16_t height )
{
    if ( m_TextureWidth != width || m_TextureHeight != height )
    {
        // Release resource before resizing
        m_pTexture2D.Reset();
        m_pRenderTargetView.Reset();
        m_pDepthStencilView.Reset();
        m_pShaderResourceView.Reset();
        m_pUnorderedAccessView.Reset();

        m_TextureWidth = glm::max<uint16_t>( width, 1 );
        m_TextureHeight = glm::max<uint16_t>( height, 1 );

        // Create texture with the dimensions specified.
        D3D11_TEXTURE2D_DESC textureDesc = { 0 };

        textureDesc.ArraySize = m_NumSlices;
        textureDesc.Format = m_TextureResourceFormat;
        textureDesc.SampleDesc = m_SampleDesc;

        textureDesc.Width = m_TextureWidth;
        textureDesc.Height = m_TextureHeight;
        textureDesc.MipLevels = 1;

        if ( ( (int)m_CPUAccess & (int)CPUAccess::Read ) != 0 )
        {
            textureDesc.Usage = D3D11_USAGE_STAGING;
            textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
        }
        else if ( ( (int)m_CPUAccess & (int)CPUAccess::Write ) != 0 )
        {
            textureDesc.Usage = D3D11_USAGE_DYNAMIC;
            textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        }
        else
        {
            textureDesc.Usage = D3D11_USAGE_DEFAULT;
            textureDesc.CPUAccessFlags = 0;
        }

        if ( !m_bUAV && !m_bDynamic && ( m_DepthStencilViewFormatSupport & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL ) != 0 )
        {
            textureDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        }
        if ( !m_bDynamic && ( m_RenderTargetViewFormatSupport & D3D11_FORMAT_SUPPORT_RENDER_TARGET ) != 0 )
        {
            textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        }
        if ( ( (int)m_CPUAccess & (int)CPUAccess::Read ) == 0  )
        {
            textureDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
        if ( m_bUAV && !m_bDynamic )
        {
            textureDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        textureDesc.MiscFlags = m_bGenerateMipmaps ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

        if ( FAILED( m_pDevice->CreateTexture2D( &textureDesc, nullptr, &m_pTexture2D ) ) )
        {
            ReportError( "Failed to create texture." );
            return;
        }

        if ( ( textureDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL ) != 0 )
        {
            // Create the depth/stencil view for the texture.
            D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
            depthStencilViewDesc.Format = m_DepthStencilViewFormat;
            depthStencilViewDesc.Flags = 0;

            if ( m_NumSlices > 1 )
            {
                if ( m_SampleDesc.Count > 1 )
                {
                    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                    depthStencilViewDesc.Texture2DMSArray.FirstArraySlice = 0;
                    depthStencilViewDesc.Texture2DMSArray.ArraySize = m_NumSlices;
                }
                else
                {
                    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    depthStencilViewDesc.Texture2DArray.MipSlice = 0;
                    depthStencilViewDesc.Texture2DArray.FirstArraySlice = 0;
                    depthStencilViewDesc.Texture2DArray.ArraySize = m_NumSlices;
                }
            }
            else
            {
                if ( m_SampleDesc.Count > 1 )
                {
                    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
                }
                else
                {
                    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    depthStencilViewDesc.Texture2D.MipSlice = 0;
                }
            }

            if ( FAILED( m_pDevice->CreateDepthStencilView( m_pTexture2D.Get(), &depthStencilViewDesc, &m_pDepthStencilView ) ) )
            {
                ReportError( "Failed to create depth/stencil view." );
            }
        }
        
        if ( ( textureDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE ) != 0 )
        {
            // Create a Shader resource view for the texture.
            D3D11_SHADER_RESOURCE_VIEW_DESC resourceViewDesc;
            resourceViewDesc.Format = m_ShaderResourceViewFormat;

            if ( m_NumSlices > 1 )
            {
                if ( m_SampleDesc.Count > 1 )
                {
                    resourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                    resourceViewDesc.Texture2DMSArray.FirstArraySlice = 0;
                    resourceViewDesc.Texture2DMSArray.ArraySize = m_NumSlices;
                }
                else
                {
                    resourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    resourceViewDesc.Texture2DArray.FirstArraySlice = 0;
                    resourceViewDesc.Texture2DArray.ArraySize = m_NumSlices;
                    resourceViewDesc.Texture2DArray.MipLevels = m_bGenerateMipmaps ? -1 : 1;
                    resourceViewDesc.Texture2DArray.MostDetailedMip = 0;
                }
            }
            else
            {
                if ( m_SampleDesc.Count > 1 )
                {
                    resourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
                }
                else
                {
                    resourceViewDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
                    resourceViewDesc.Texture2D.MipLevels = m_bGenerateMipmaps ? -1 : 1;
                    resourceViewDesc.Texture2D.MostDetailedMip = 0;
                }
            }

            if ( FAILED( m_pDevice->CreateShaderResourceView( m_pTexture2D.Get(), &resourceViewDesc, &m_pShaderResourceView ) ) )
            {
                ReportError( "Failed to create texture resource view." );
            } 
            else if ( m_bGenerateMipmaps )
            {
                m_pDeviceContext->GenerateMips( m_pShaderResourceView.Get() );
            }
        }

        if ( ( textureDesc.BindFlags & D3D11_BIND_RENDER_TARGET ) != 0 )
        {
            // Create the render target view for the texture.
            D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
            renderTargetViewDesc.Format = m_RenderTargetViewFormat;

            if ( m_NumSlices > 1 )
            {
                if ( m_SampleDesc.Count > 1 )
                {
                    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    renderTargetViewDesc.Texture2DArray.FirstArraySlice = 0;
                    renderTargetViewDesc.Texture2DArray.ArraySize = m_NumSlices;

                }
                else
                {
                    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    renderTargetViewDesc.Texture2DArray.MipSlice = 0;
                    renderTargetViewDesc.Texture2DArray.FirstArraySlice = 0;
                    renderTargetViewDesc.Texture2DArray.ArraySize = m_NumSlices;
                }
            }
            else
            {
                if ( m_SampleDesc.Count > 1 )
                {
                    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                }
                else
                {
                    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    renderTargetViewDesc.Texture2D.MipSlice = 0;
                }
            }

            if ( FAILED( m_pDevice->CreateRenderTargetView( m_pTexture2D.Get(), &renderTargetViewDesc, &m_pRenderTargetView ) ) )
            {
                ReportError( "Failed to create render target view." );
            }
        }

        if ( ( textureDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS ) != 0 )
        {
            // UAVs cannot be multi sampled.
            assert( m_SampleDesc.Count == 1 );

            // Create a Shader resource view for the texture.
            D3D11_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc;
            unorderedAccessViewDesc.Format = m_UnorderedAccessViewFormat;

            if ( m_NumSlices > 1 )
            {
                unorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                unorderedAccessViewDesc.Texture2DArray.MipSlice = 0;
                unorderedAccessViewDesc.Texture2DArray.FirstArraySlice = 0;
                unorderedAccessViewDesc.Texture2DArray.ArraySize = m_NumSlices;
            }
            else
            {
                unorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                unorderedAccessViewDesc.Texture2D.MipSlice = 0;
            }

            if ( FAILED( m_pDevice->CreateUnorderedAccessView( m_pTexture2D.Get(), &unorderedAccessViewDesc, &m_pUnorderedAccessView) ) )
            {
                ReportError( "Failed to create unordered access view." );
            }
        }

        assert( m_BPP > 0 && m_BPP % 8 == 0 );
        m_Buffer.resize( width * height * ( m_BPP / 8 ) );
    }
}

void TextureDX11::Resize3D( uint16_t width, uint16_t height, uint16_t depth )
{
    // TODO
}

void TextureDX11::ResizeCube( uint16_t size )
{
    // TODO
}

void TextureDX11::Resize( uint16_t width, uint16_t height, uint16_t depth )
{
    switch ( m_TextureDimension )
    {
    case Dimension::Texture1D:
    case Dimension::Texture1DArray:
        Resize1D( width );
        break;
    case Dimension::Texture2D:
    case Dimension::Texture2DArray:
        Resize2D( width, height );
        break;
    case Dimension::Texture3D:
        Resize3D( width, height, depth );
        break;
    case Texture::Dimension::TextureCube:
        ResizeCube( width );
        break;
    default:
        ReportError( "Unknown texture dimension." );
        break;
    }

    return;
}

void TextureDX11::Plot( glm::ivec2 coord, const uint8_t* pixel, size_t size )
{
    assert( m_BPP > 0 && m_BPP % 8 == 0 );
    assert( coord.s < m_TextureWidth && coord.t < m_TextureHeight && size == ( m_BPP / 8 ) );

    uint8_t bytesPerPixel = ( m_BPP / 8 );
    uint32_t stride = m_TextureWidth * bytesPerPixel;
    uint32_t index = ( coord.s * bytesPerPixel ) + ( coord.t * stride );

    for ( unsigned int i = 0; i < size; ++i )
    {
        m_Buffer[index + i] = *( pixel + i );
    }

    m_bIsDirty = true;
}

void TextureDX11::FetchPixel( glm::ivec2 coord, uint8_t*& pixel, size_t size )
{
    assert( m_BPP > 0 && m_BPP % 8 == 0 );
    assert( coord.s < m_TextureWidth && coord.t < m_TextureHeight && size == ( m_BPP / 8 ) );

    uint8_t bytesPerPixel = ( m_BPP / 8 );
    uint32_t stride = m_TextureWidth * bytesPerPixel;
    uint32_t index = ( coord.s * bytesPerPixel ) + ( coord.t * stride );
    pixel = &m_Buffer[index];
}

void TextureDX11::Copy( std::shared_ptr<Texture> other )
{
    std::shared_ptr<TextureDX11> srcTexture = std::dynamic_pointer_cast<TextureDX11>( other );

    if ( srcTexture && srcTexture.get() != this )
    {
        if ( m_TextureDimension == srcTexture->m_TextureDimension &&
             m_TextureWidth == srcTexture->m_TextureWidth &&
             m_TextureHeight == srcTexture->m_TextureHeight )
        {
            switch ( m_TextureDimension )
            {
            case Dimension::Texture1D:
            case Dimension::Texture1DArray:
                m_pDeviceContext->CopyResource( m_pTexture1D.Get(), srcTexture->m_pTexture1D.Get() );
                break;
            case Texture::Dimension::Texture2D:
            case Texture::Dimension::Texture2DArray:
                m_pDeviceContext->CopyResource( m_pTexture2D.Get(), srcTexture->m_pTexture2D.Get() );
                break;
            case Texture::Dimension::Texture3D:
            case Texture::Dimension::TextureCube:
                m_pDeviceContext->CopyResource( m_pTexture3D.Get(), srcTexture->m_pTexture3D.Get() );
                break;
            }
        }
        else
        {
            ReportError( "Incompatible source texture." );
        }
    }

    if ( ( (int)m_CPUAccess & (int)CPUAccess::Read ) != 0 && m_pTexture2D )
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;

        // Copy the texture data from the texture resource
        if ( FAILED( m_pDeviceContext->Map( m_pTexture2D.Get(), 0, D3D11_MAP_READ, 0, &mappedResource ) ) )
        {
            ReportError( "Failed to map texture resource for reading." );
        }

        memcpy_s( m_Buffer.data(), m_Buffer.size(), mappedResource.pData, m_Buffer.size() );

        m_pDeviceContext->Unmap( m_pTexture2D.Get(), 0 );
    }
}

void TextureDX11::Clear( ClearFlags clearFlags, const glm::vec4& color, float depth, uint8_t stencil )
{
    if ( m_pRenderTargetView && ( (int)clearFlags & (int)ClearFlags::Color ) != 0 )
    {
        m_pDeviceContext->ClearRenderTargetView( m_pRenderTargetView.Get(), glm::value_ptr( color ) );
    }

    {
        UINT flags = 0;
        flags |= ( (int)clearFlags & (int)ClearFlags::Depth ) != 0 ? D3D11_CLEAR_DEPTH : 0;
        flags |= ( (int)clearFlags & (int)ClearFlags::Stencil ) != 0 ? D3D11_CLEAR_STENCIL : 0;
        if ( m_pDepthStencilView && flags > 0 )
        {
            m_pDeviceContext->ClearDepthStencilView( m_pDepthStencilView.Get(), flags, depth, stencil );
        }
    }
}

void TextureDX11::OnFileChanged( FileChangeEventArgs& e )
{
    m_bFileChanged = true;
}

void TextureDX11::Bind( uint32_t ID, Shader::ShaderType shaderType, ShaderParameter::Type parameterType )
{
    if ( m_bFileChanged )
    {
        MutexLock lock( m_Mutex );
        if ( m_DependencyTracker.IsStale() )
        {
            if ( m_TextureDimension == Texture::Dimension::Texture2D )
            {
                LoadTexture2D( m_TextureFileName );
            }
            else if ( m_TextureDimension == Texture::Dimension::TextureCube )
            {
                LoadTextureCube( m_TextureFileName );
            }
        }

        m_bFileChanged = false;
    }

    if ( m_bIsDirty ) 
    {
        if ( m_bDynamic && m_pTexture2D )
        {
            D3D11_MAPPED_SUBRESOURCE mappedResource;

            // Copy the texture data to the texture resource
            HRESULT hr = m_pDeviceContext->Map( m_pTexture2D.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
            if ( FAILED( hr ) )
            {
                ReportError( "Failed to map texture resource for writing." );
            }

            memcpy_s( mappedResource.pData, m_Buffer.size(), m_Buffer.data(), m_Buffer.size() );

            m_pDeviceContext->Unmap( m_pTexture2D.Get(), 0 );

            if ( m_bGenerateMipmaps )
            {
                m_pDeviceContext->GenerateMips( m_pShaderResourceView.Get() );
            }
        }
        m_bIsDirty = false;
    }

    ID3D11ShaderResourceView* srv[] = { m_pShaderResourceView.Get() };
    ID3D11UnorderedAccessView* uav[] = { m_pUnorderedAccessView.Get() };

    if ( parameterType == ShaderParameter::Type::Texture && m_pShaderResourceView )
    {
        switch ( shaderType )
        {
        case Shader::VertexShader:
            m_pDeviceContext->VSSetShaderResources( ID, 1, srv );
            break;
        case Shader::TessellationControlShader:
            m_pDeviceContext->HSSetShaderResources( ID, 1, srv );
            break;
        case Shader::TessellationEvaluationShader:
            m_pDeviceContext->DSSetShaderResources( ID, 1, srv );
            break;
        case Shader::GeometryShader:
            m_pDeviceContext->GSSetShaderResources( ID, 1, srv );
            break;
        case Shader::PixelShader:
            m_pDeviceContext->PSSetShaderResources( ID, 1, srv );
            break;
        case Shader::ComputeShader:
            m_pDeviceContext->CSSetShaderResources( ID, 1, srv );
            break;
        }
    }
    else if ( parameterType == ShaderParameter::Type::RWTexture && m_pUnorderedAccessView )
    {
        switch ( shaderType )
        {
        case Shader::ComputeShader:
            m_pDeviceContext->CSSetUnorderedAccessViews( ID, 1, uav, nullptr );
            break;
        }
    }

}
void TextureDX11::UnBind( uint32_t ID, Shader::ShaderType shaderType, ShaderParameter::Type parameterType )
{
    ID3D11ShaderResourceView* srv[] = { nullptr };
    ID3D11UnorderedAccessView* uav[] = { nullptr };

    if ( parameterType == ShaderParameter::Type::Texture )
    {
        switch ( shaderType )
        {
        case Shader::VertexShader:
            m_pDeviceContext->VSSetShaderResources( ID, 1, srv );
            break;
        case Shader::TessellationControlShader:
            m_pDeviceContext->HSSetShaderResources( ID, 1, srv );
            break;
        case Shader::TessellationEvaluationShader:
            m_pDeviceContext->DSSetShaderResources( ID, 1, srv );
            break;
        case Shader::GeometryShader:
            m_pDeviceContext->GSSetShaderResources( ID, 1, srv );
            break;
        case Shader::PixelShader:
            m_pDeviceContext->PSSetShaderResources( ID, 1, srv );
            break;
        case Shader::ComputeShader:
            m_pDeviceContext->CSSetShaderResources( ID, 1, srv );
            break;
        }
    }
    else if ( parameterType == ShaderParameter::Type::RWTexture )
    {
        switch ( shaderType )
        {
        case Shader::ComputeShader:
            m_pDeviceContext->CSSetUnorderedAccessViews( ID, 1, uav, nullptr );
            break;
        }
    }
}

DXGI_FORMAT TextureDX11::TranslateFormat( const TextureFormat& format )
{
    DXGI_FORMAT result = DXGI_FORMAT_UNKNOWN;
    
    switch ( format.Components )
    {
    case Components::R:
        switch ( format.Type )
        {
        case Type::Typeless:
            if ( format.RedBits == 8 )
            {
                result = DXGI_FORMAT_R8_TYPELESS;
            }
            else if ( format.RedBits == 16 )
            {
                result = DXGI_FORMAT_R16_TYPELESS;
            }
            else if ( format.RedBits == 32 )
            {
                result = DXGI_FORMAT_R32_TYPELESS;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose the best format based on the requested format.
                if ( format.RedBits > 16 )
                {
                    result = DXGI_FORMAT_R32_TYPELESS;
                }
                else if ( format.RedBits > 8 )
                {
                    result = DXGI_FORMAT_R16_TYPELESS;
                }
                else
                {
                    result = DXGI_FORMAT_R8_TYPELESS;
                }
            }
            break;
        case Type::UnsignedNormalized:
            if ( format.RedBits == 1 )
            {
                result = DXGI_FORMAT_R1_UNORM;
            }
            else if ( format.RedBits == 8 )
            {
                result = DXGI_FORMAT_R8_UNORM;
            }
            else if ( format.RedBits == 16 )
            {
                result = DXGI_FORMAT_R16_UNORM;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative to the requested format.
                if ( format.RedBits > 8 )
                {
                    result = DXGI_FORMAT_R16_UNORM;
                }
                else if ( format.RedBits > 1 )
                {
                    result = DXGI_FORMAT_R8_UNORM;
                }
                else
                {
                    result = DXGI_FORMAT_R1_UNORM;
                }
            }
            break;
        case Type::SignedNormalized:
            if ( format.RedBits == 8 )
            {
                result = DXGI_FORMAT_R8_SNORM;
            }
            else if ( format.RedBits == 16 )
            {
                result = DXGI_FORMAT_R16_SNORM;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative.
                if ( format.RedBits > 8 )
                {
                    result = DXGI_FORMAT_R16_SNORM;
                }
                else
                {
                    result = DXGI_FORMAT_R8_SNORM;
                }
            }
            break;
        case Type::Float:
            if ( format.RedBits == 16 )
            {
                result = DXGI_FORMAT_R16_FLOAT;
            }
            else if ( format.RedBits == 32 )
            {
                result = DXGI_FORMAT_R32_FLOAT;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative.
                if ( format.RedBits > 16 )
                {
                    result = DXGI_FORMAT_R32_FLOAT;
                }
                else
                {
                    result = DXGI_FORMAT_R16_FLOAT;
                }
            }
            break;
        case Type::UnsignedInteger:
            if ( format.RedBits == 8 )
            {
                result = DXGI_FORMAT_R8_UINT;
            }
            else if ( format.RedBits == 16 )
            {
                result = DXGI_FORMAT_R16_UINT;
            }
            else if ( format.RedBits == 32 )
            {
                result = DXGI_FORMAT_R32_UINT;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative.
                if ( format.RedBits > 16 )
                {
                    result = DXGI_FORMAT_R32_UINT;
                }
                else if ( format.RedBits > 8 )
                {
                    result = DXGI_FORMAT_R16_UINT;
                }
                else
                {
                    result = DXGI_FORMAT_R8_UINT;
                }
            }
            break;
        case Type::SignedInteger:
            if ( format.RedBits == 8 )
            {
                result = DXGI_FORMAT_R8_SINT;
            }
            else if ( format.RedBits == 16 )
            {
                result = DXGI_FORMAT_R16_SINT;
            }
            else if ( format.RedBits == 32 )
            {
                result = DXGI_FORMAT_R32_SINT;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative
                if ( format.RedBits > 16 )
                {
                    result = DXGI_FORMAT_R32_SINT;
                }
                else if ( format.RedBits > 8 )
                {
                    result = DXGI_FORMAT_R16_SINT;
                }
                else
                {
                    result = DXGI_FORMAT_R8_SINT;
                }
            }
            break;
        default:
            ReportTextureFormatError( format, "Unknown texture format." );
            break;
        }
        break;
    case Components::RG:
        switch ( format.Type )
        {
        case Type::Typeless:
            if ( format.RedBits == 8 && format.GreenBits == 8 )
            {
                result = DXGI_FORMAT_R8G8_TYPELESS;
            }
            else if ( format.RedBits == 16 && format.GreenBits == 16 )
            {
                result = DXGI_FORMAT_R16G16_TYPELESS;
            }
            else if ( format.RedBits == 24 && format.GreenBits == 8 )
            {
                result = DXGI_FORMAT_R24G8_TYPELESS;
            }
            else if ( format.RedBits == 32 && format.GreenBits == 32 )
            {
                result = DXGI_FORMAT_R32G32_TYPELESS;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose the best format based on the requested format.
                if ( format.RedBits > 24 || format.GreenBits > 16 )
                {
                    result = DXGI_FORMAT_R32G32_TYPELESS;
                }
                else if ( format.RedBits > 16 && format.GreenBits <= 8 )
                {
                    result = DXGI_FORMAT_R24G8_TYPELESS;
                }
                else if ( format.RedBits > 8 || format.GreenBits > 8 )
                {
                    result = DXGI_FORMAT_R16G16_TYPELESS;
                }
                else
                {
                    result = DXGI_FORMAT_R8G8_TYPELESS;
                }
            }
            break;
        case Type::UnsignedNormalized:
            if ( format.RedBits == 8 && format.GreenBits == 8 )
            {
                result = DXGI_FORMAT_R8G8_UNORM;
            }
            else if ( format.RedBits == 16 && format.GreenBits == 16 )
            {
                result = DXGI_FORMAT_R16G16_UNORM;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative.
                if ( format.RedBits > 8 || format.GreenBits > 8 )
                {
                    result = DXGI_FORMAT_R16G16_UNORM;
                }
                else
                {
                    result = DXGI_FORMAT_R8G8_UNORM;
                }
            }
            break;
        case Type::SignedNormalized:
            if ( format.RedBits == 8 && format.GreenBits == 8 )
            {
                result = DXGI_FORMAT_R8G8_SNORM;
            }
            else if ( format.RedBits == 16 && format.GreenBits == 16 )
            {
                result = DXGI_FORMAT_R16G16_SNORM;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative.
                if ( format.RedBits > 8 || format.GreenBits > 8 )
                {
                    result = DXGI_FORMAT_R16G16_SNORM;
                }
                else
                {
                    result = DXGI_FORMAT_R8G8_SNORM;
                }
            }
            break;
        case Type::Float:
            if ( format.RedBits == 16 && format.GreenBits == 16 )
            {
                result = DXGI_FORMAT_R16G16_FLOAT;
            }
            else if ( format.RedBits == 32 && format.GreenBits == 32 )
            {
                result = DXGI_FORMAT_R32G32_FLOAT;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative.
                if ( format.RedBits > 16 || format.GreenBits > 16 )
                {
                    result = DXGI_FORMAT_R32G32_FLOAT;
                }
                else
                {
                    result = DXGI_FORMAT_R16G16_FLOAT;
                }
            }
            break;
        case Type::UnsignedInteger:
            if ( format.RedBits == 8 && format.GreenBits == 8 )
            {
                result = DXGI_FORMAT_R8G8_UINT;
            }
            else if ( format.RedBits == 16 && format.GreenBits == 16 )
            {
                result = DXGI_FORMAT_R16G16_UINT;
            }
            else if ( format.RedBits == 32 && format.GreenBits == 32 )
            {
                result = DXGI_FORMAT_R32G32_UINT;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative.
                if ( format.RedBits > 16 || format.GreenBits > 16 )
                {
                    result = DXGI_FORMAT_R32G32_UINT;
                }
                else if ( format.RedBits > 8 || format.GreenBits > 8 )
                {
                    result = DXGI_FORMAT_R16G16_UINT;
                }
                else
                {
                    result = DXGI_FORMAT_R8G8_UINT;
                }
            }
            break;
        case Type::SignedInteger:
            if ( format.RedBits == 8 && format.GreenBits == 8 )
            {
                result = DXGI_FORMAT_R8G8_SINT;
            }
            else if ( format.RedBits == 16 && format.GreenBits == 16 )
            {
                result = DXGI_FORMAT_R16G16_SINT;
            }
            else if ( format.RedBits == 32 && format.GreenBits == 32 )
            {
                result = DXGI_FORMAT_R32G32_SINT;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative.
                if ( format.RedBits > 16 || format.GreenBits > 16 )
                {
                    result = DXGI_FORMAT_R32G32_SINT;
                }
                else if ( format.RedBits > 8 || format.GreenBits > 8 )
                {
                    result = DXGI_FORMAT_R16G16_SINT;
                }
                else
                {
                    result = DXGI_FORMAT_R8G8_SINT;
                }
            }
            break;
        default:
            ReportTextureFormatError( format, "Unknown texture format." );
            break;
        }
        break;
    case Components::RGB:
        switch ( format.Type )
        {
        case Type::Typeless:
            if ( format.RedBits == 32 && format.GreenBits == 32 && format.BlueBits == 32 )
            {
                result = DXGI_FORMAT_R32G32B32_TYPELESS;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // There is only 1 RGB typeless format
                result = DXGI_FORMAT_R32G32B32_TYPELESS;
            }
            break;
        case Type::Float:
            if ( format.RedBits == 11 && format.GreenBits == 11 && format.BlueBits == 10 )
            {
                result = DXGI_FORMAT_R11G11B10_FLOAT;
            }
            else if ( format.RedBits == 32 && format.GreenBits == 32 && format.BlueBits == 32 )
            {
                result = DXGI_FORMAT_R32G32B32_FLOAT;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative.
                if ( format.RedBits > 11 || format.GreenBits > 11 || format.BlueBits > 10 )
                {
                    result = DXGI_FORMAT_R32G32B32_FLOAT;
                }
                else
                {
                    result = DXGI_FORMAT_R11G11B10_FLOAT;
                }
            }
            break;
        case Type::UnsignedInteger:
            if ( format.RedBits == 32 && format.GreenBits == 32 && format.BlueBits == 32 )
            {
                result = DXGI_FORMAT_R32G32B32_UINT;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // There is only 1 3-component UINT format.
                result = DXGI_FORMAT_R32G32B32_UINT;
            }
            break;
        case Type::SignedInteger:
            if ( format.RedBits == 32 && format.GreenBits == 32 && format.BlueBits == 32 )
            {
                result = DXGI_FORMAT_R32G32B32_SINT;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // There is only 1 3-component SINT format.
                result = DXGI_FORMAT_R32G32B32_SINT;
            }
            break;
        default:
            ReportTextureFormatError( format, "Unsupported texture format." );
            // Try to choose a reasonable alternative
            switch ( format.Type )
            {
            case Type::UnsignedNormalized:
                // This is a non-normalized format. May result in unintended behavior.
                result = DXGI_FORMAT_R32G32B32_UINT;
                break;
            case Type::SignedNormalized:
                // Non-normalized format. May result in unintended behavior.
                result = DXGI_FORMAT_R32G32B32_SINT;
                break;
            default:
                ReportTextureFormatError( format, "Unknown texture format." );
                break;
            }
            break;
        }
        break;
    case Components::RGBA:
        switch ( format.Type )
        {
        case Type::Typeless:
            if ( format.RedBits == 8 && format.GreenBits == 8 && format.BlueBits == 8 && format.AlphaBits == 8 )
            {
                result = DXGI_FORMAT_R8G8B8A8_TYPELESS;
            }
            else if ( format.RedBits == 10 && format.GreenBits == 10 && format.BlueBits == 10 && format.AlphaBits == 2 )
            {
                result = DXGI_FORMAT_R10G10B10A2_TYPELESS;
            }
            else if ( format.RedBits == 16 && format.GreenBits == 16 && format.BlueBits == 16 && format.AlphaBits == 16 )
            {
                result = DXGI_FORMAT_R16G16B16A16_TYPELESS;
            }
            else if ( format.RedBits == 32 && format.GreenBits == 32 && format.BlueBits == 32 && format.AlphaBits == 32 )
            {
                result = DXGI_FORMAT_R32G32B32A32_TYPELESS;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose the best format based on the requested format.
                if ( format.RedBits > 16 || format.GreenBits > 16 || format.BlueBits > 16 || format.AlphaBits > 16 )
                {
                    result = DXGI_FORMAT_R32G32B32A32_TYPELESS;
                }
                else if ( ( format.RedBits > 10 || format.GreenBits > 10 || format.BlueBits > 10 ) && format.AlphaBits <= 2 )
                {
                    result = DXGI_FORMAT_R10G10B10A2_TYPELESS;
                }
                else if ( format.RedBits > 8 || format.GreenBits > 8 || format.BlueBits > 8 || format.AlphaBits > 8 )
                {
                    result = DXGI_FORMAT_R16G16B16A16_TYPELESS;
                }
                else
                {
                    result = DXGI_FORMAT_R8G8B8A8_TYPELESS;
                }
            }
            break;
        case Type::UnsignedNormalized:
            if ( format.RedBits == 8 && format.GreenBits == 8 && format.BlueBits == 8 && format.AlphaBits == 8 )
            {
                result = DXGI_FORMAT_R8G8B8A8_UNORM;
            }
            else if ( format.RedBits == 10 && format.GreenBits == 10 && format.BlueBits == 10 && format.AlphaBits == 2 )
            {
                result = DXGI_FORMAT_R10G10B10A2_UNORM;
            }
            else if ( format.RedBits == 16 && format.GreenBits == 16 && format.BlueBits == 16 && format.AlphaBits == 16 )
            {
                result = DXGI_FORMAT_R16G16B16A16_UNORM;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative.
                if ( format.RedBits > 10 || format.GreenBits > 10 || format.BlueBits > 10 || format.AlphaBits > 8 )
                {
                    result = DXGI_FORMAT_R16G16B16A16_UNORM;
                }
                else if ( ( format.RedBits > 8 || format.GreenBits > 8 || format.BlueBits > 8 ) && format.AlphaBits <= 2 )
                {
                    result = DXGI_FORMAT_R10G10B10A2_UNORM;
                }
                else
                {
                    result = DXGI_FORMAT_R8G8B8A8_UNORM;
                }
            }
            break;
        case Type::SignedNormalized:
            if ( format.RedBits == 8 && format.GreenBits == 8 && format.BlueBits == 8 && format.AlphaBits == 8 )
            {
                result = DXGI_FORMAT_R8G8B8A8_SNORM;
            }
            else if ( format.RedBits == 16 && format.GreenBits == 16 && format.BlueBits == 16 && format.AlphaBits == 16 )
            {
                result = DXGI_FORMAT_R16G16B16A16_SNORM;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative.
                if ( format.RedBits > 8 || format.GreenBits > 8 || format.BlueBits > 8 || format.AlphaBits > 8 )
                {
                    result = DXGI_FORMAT_R16G16B16A16_SNORM;
                }
                else
                {
                    result = DXGI_FORMAT_R8G8B8A8_SNORM;
                }
            }
            break;
        case Type::Float:
            if ( format.RedBits == 32 && format.GreenBits == 32 && format.BlueBits && format.AlphaBits == 32 )
            {
                result = DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
            else if ( format.RedBits == 16 && format.GreenBits == 16 && format.BlueBits == 16 && format.AlphaBits == 16 )
            {
                result = DXGI_FORMAT_R16G16B16A16_FLOAT;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative.
                if ( format.RedBits > 16 || format.GreenBits > 16 || format.BlueBits > 16 || format.AlphaBits > 16 )
                {
                    result = DXGI_FORMAT_R32G32B32A32_FLOAT;
                }
                else
                {
                    result = DXGI_FORMAT_R16G16B16A16_FLOAT;
                }
            }
            break;
        case Type::UnsignedInteger:
            if ( format.RedBits == 8 && format.GreenBits == 8 && format.BlueBits == 8 && format.AlphaBits == 8 )
            {
                result = DXGI_FORMAT_R8G8B8A8_UINT;
            }
            else if ( format.RedBits == 10 && format.GreenBits == 10 && format.BlueBits == 10 && format.AlphaBits == 2 )
            {
                result = DXGI_FORMAT_R10G10B10A2_UINT;
            }
            else if ( format.RedBits == 16 && format.GreenBits == 16 && format.BlueBits == 16 && format.AlphaBits == 16 )
            {
                result = DXGI_FORMAT_R16G16B16A16_UINT;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative.
                if ( format.RedBits > 10 || format.GreenBits > 10 || format.BlueBits > 10 || format.AlphaBits > 10 )
                {
                    result = DXGI_FORMAT_R16G16B16A16_UINT;
                }
                else if ( ( format.RedBits > 8 || format.GreenBits > 8 || format.BlueBits > 8 ) && format.AlphaBits <= 2 )
                {
                    result = DXGI_FORMAT_R10G10B10A2_UINT;
                }
                else
                {
                    result = DXGI_FORMAT_R8G8B8A8_UINT;
                }
            }
            break;
        case Type::SignedInteger:
            if ( format.RedBits == 8 && format.GreenBits == 8 && format.BlueBits == 8 && format.AlphaBits == 8 )
            {
                result = DXGI_FORMAT_R8G8B8A8_SINT;
            }
            else if ( format.RedBits == 16 && format.GreenBits == 16 && format.BlueBits == 16 && format.AlphaBits == 16 )
            {
                result = DXGI_FORMAT_R16G16B16A16_SINT;
            }
            else if ( format.RedBits == 32 && format.GreenBits == 32 && format.BlueBits == 32 && format.AlphaBits == 32 )
            {
                result = DXGI_FORMAT_R32G32B32A32_SINT;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Try to choose a reasonable alternative.
                if ( format.RedBits > 16 || format.GreenBits > 16 || format.BlueBits > 16 || format.AlphaBits > 16 )
                {
                    result = DXGI_FORMAT_R32G32B32A32_SINT;
                }
                if ( format.RedBits > 8 || format.GreenBits > 8 || format.BlueBits > 8 || format.AlphaBits > 8 )
                {
                    result = DXGI_FORMAT_R16G16B16A16_SINT;
                }
                else
                {
                    result = DXGI_FORMAT_R8G8B8A8_SINT;
                }
            }
            break;
        default:
            ReportTextureFormatError( format, "Unknown texture format." );
            break;
        }
        break;
    case Components::Depth:
        switch ( format.Type )
        {
        case Type::UnsignedNormalized:
            if ( format.DepthBits == 16 )
            {
                result = DXGI_FORMAT_D16_UNORM;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Only 1 format that could match.
                result = DXGI_FORMAT_D16_UNORM;
            }
            break;
        case Type::Float:
            if ( format.DepthBits == 32 )
            {
                result = DXGI_FORMAT_D32_FLOAT;
            }
            else
            {
                ReportTextureFormatError( format, "Unsupported texture format." );
                // Only 1 format that could match.
                result = DXGI_FORMAT_D32_FLOAT;
            }
            break;
        default:
            // There are no SNORM, SINT, or UINT depth-only formats.
            ReportTextureFormatError( format, "Unknown texture format." );
            break;
        }
        break;
    case Components::DepthStencil:
        // For Depth/Stencil formats, we'll ignore the type and try to deduce the format
        // based on the bit-depth values.
        if ( format.DepthBits == 24 && format.StencilBits == 8 )
        {
            result = DXGI_FORMAT_D24_UNORM_S8_UINT;
        }
        else if ( format.DepthBits == 32 && format.StencilBits == 8 )
        {
            result = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        }
        else
        {
            ReportTextureFormatError( format, "Unsupported texture format." );
            // Try to choose a reasonable alternative...
            if ( format.DepthBits > 24 )
            {
                result = result = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            }
            else 
            {
                result = DXGI_FORMAT_D24_UNORM_S8_UINT;
            }
        }
        break;
    default:
        ReportTextureFormatError( format, "Unknown texture format." );
        break;
    }
    
    return result;
}

DXGI_FORMAT TextureDX11::GetTextureFormat( DXGI_FORMAT format )
{
    DXGI_FORMAT result = format;

    switch ( format )
    {
    case DXGI_FORMAT_D16_UNORM:
        result = DXGI_FORMAT_R16_TYPELESS;
        break;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        result = DXGI_FORMAT_R24G8_TYPELESS;
        break;
    case DXGI_FORMAT_D32_FLOAT:
        result = DXGI_FORMAT_R32_TYPELESS;
        break;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        result = DXGI_FORMAT_R32G8X24_TYPELESS;
        break;
    }

    return result;
}

DXGI_FORMAT TextureDX11::GetDSVFormat( DXGI_FORMAT format )
{
    DXGI_FORMAT result = format;

    switch ( format )
    {
    case DXGI_FORMAT_R16_TYPELESS:
        result = DXGI_FORMAT_D16_UNORM;
        break;
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        result = DXGI_FORMAT_D24_UNORM_S8_UINT;
        break;
    case DXGI_FORMAT_R32_TYPELESS:
        result = DXGI_FORMAT_D32_FLOAT;
        break;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        result = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        break;
    }

    return result;
}

DXGI_FORMAT TextureDX11::GetSRVFormat( DXGI_FORMAT format )
{
    DXGI_FORMAT result = format;
    switch ( format )
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        result = DXGI_FORMAT_R32G32B32A32_FLOAT;
        break;
    case DXGI_FORMAT_R32G32B32_TYPELESS:
        result = DXGI_FORMAT_R32G32B32_FLOAT;
        break;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        result = DXGI_FORMAT_R16G16B16A16_UNORM;
        break;
    case DXGI_FORMAT_R32G32_TYPELESS:
        result = DXGI_FORMAT_R32G32_FLOAT;
        break;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        result = DXGI_FORMAT_R10G10B10A2_UNORM;
        break;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        result = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;
    case DXGI_FORMAT_R16G16_TYPELESS:
        result = DXGI_FORMAT_R16G16_UNORM;
        break;
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_D16_UNORM:
        result = DXGI_FORMAT_R16_UNORM;
        break;
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        result = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        break;
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
        result = DXGI_FORMAT_R32_FLOAT;
        break;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        result = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        break;
    case DXGI_FORMAT_R8G8_TYPELESS:
        result = DXGI_FORMAT_R8G8_UNORM;
        break;
    case DXGI_FORMAT_R8_TYPELESS:
        result = DXGI_FORMAT_R8_UNORM;
        break;
    }

    return result;
}

DXGI_FORMAT TextureDX11::GetRTVFormat( DXGI_FORMAT format )
{
    DXGI_FORMAT result = format;

    switch ( format )
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        result = DXGI_FORMAT_R32G32B32A32_FLOAT;
        break;
    case DXGI_FORMAT_R32G32B32_TYPELESS:
        result = DXGI_FORMAT_R32G32B32_FLOAT;
        break;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        result = DXGI_FORMAT_R16G16B16A16_UNORM;
        break;
    case DXGI_FORMAT_R32G32_TYPELESS:
        result = DXGI_FORMAT_R32G32_FLOAT;
        break;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        result = DXGI_FORMAT_R10G10B10A2_UNORM;
        break;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        result = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;
    case DXGI_FORMAT_R16G16_TYPELESS:
        result = DXGI_FORMAT_R16G16_UNORM;
        break;
    case DXGI_FORMAT_R8G8_TYPELESS:
        result = DXGI_FORMAT_R8G8_UNORM;
        break;
    case DXGI_FORMAT_R32_TYPELESS:
        result = DXGI_FORMAT_R32_FLOAT;
        break;
    case DXGI_FORMAT_R8_TYPELESS:
        result = DXGI_FORMAT_R8_UNORM;
        break;
    }

    return result;
}

DXGI_FORMAT TextureDX11::GetUAVFormat( DXGI_FORMAT format )
{
    return GetRTVFormat( format );
}

uint8_t TextureDX11::GetBPP( DXGI_FORMAT format )
{
    uint8_t bpp = 0;

    switch ( format )
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        bpp = 128;
        break;
    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        bpp = 96;
        break;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
        bpp = 64;
        break;
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
        bpp = 64;
        break;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        bpp = 64;
        break;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
        bpp = 32;
        break;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
        bpp = 32;
        break;
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
        bpp = 32;
        break;
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
        bpp = 32;
        break;
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        bpp = 32;
        break;
    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
        bpp = 16;
        break;
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
        bpp = 16;
        break;
    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
        bpp = 8;
        break;
    case DXGI_FORMAT_R1_UNORM:
        bpp = 1;
        break;
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        bpp = 32;
        break;
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
        bpp = 32;
        break;
    case DXGI_FORMAT_B5G6R5_UNORM:
        bpp = 16;
        break;
    case DXGI_FORMAT_B5G5R5A1_UNORM:
        bpp = 16;
        break;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
        bpp = 32;
        break;
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        bpp = 32;
        break;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        bpp = 32;
        break;
    default:
        ReportError( "Unsupported texture format." );
        break;
    }

    return bpp;
}


Texture::TextureFormat TextureDX11::TranslateFormat( DXGI_FORMAT format, uint8_t numSamples )
{
    TextureFormat result;
    result.NumSamples = numSamples;

    switch ( format )
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        result.Components = Components::RGBA;
        result.Type = Type::Typeless;
        result.RedBits = 32;
        result.GreenBits = 32;
        result.BlueBits = 32;
        result.AlphaBits = 32;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        result.Components = Components::RGBA;
        result.Type = Type::Float;
        result.RedBits = 32;
        result.GreenBits = 32;
        result.BlueBits = 32;
        result.AlphaBits = 32;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R32G32B32A32_UINT:
        result.Components = Components::RGBA;
        result.Type = Type::UnsignedInteger;
        result.RedBits = 32;
        result.GreenBits = 32;
        result.BlueBits = 32;
        result.AlphaBits = 32;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R32G32B32A32_SINT:
        result.Components = Components::RGBA;
        result.Type = Type::SignedInteger;
        result.RedBits = 32;
        result.GreenBits = 32;
        result.BlueBits = 32;
        result.AlphaBits = 32;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R32G32B32_TYPELESS:
        result.Components = Components::RGB;
        result.Type = Type::Typeless;
        result.RedBits = 32;
        result.GreenBits = 32;
        result.BlueBits = 32;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R32G32B32_FLOAT:
        result.Components = Components::RGB;
        result.Type = Type::Float;
        result.RedBits = 32;
        result.GreenBits = 32;
        result.BlueBits = 32;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R32G32B32_UINT:
        result.Components = Components::RGB;
        result.Type = Type::UnsignedInteger;
        result.RedBits = 32;
        result.GreenBits = 32;
        result.BlueBits = 32;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R32G32B32_SINT:
        result.Components = Components::RGB;
        result.Type = Type::SignedInteger;
        result.RedBits = 32;
        result.GreenBits = 32;
        result.BlueBits = 32;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        result.Components = Components::RGBA;
        result.Type = Type::Typeless;
        result.RedBits = 16;
        result.GreenBits = 16;
        result.BlueBits = 16;
        result.AlphaBits = 16;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        result.Components = Components::RGBA;
        result.Type = Type::Float;
        result.RedBits = 16;
        result.GreenBits = 16;
        result.BlueBits = 16;
        result.AlphaBits = 16;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16G16B16A16_UINT:
        result.Components = Components::RGBA;
        result.Type = Type::UnsignedInteger;
        result.RedBits = 16;
        result.GreenBits = 16;
        result.BlueBits = 16;
        result.AlphaBits = 16;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16G16B16A16_SNORM:
        result.Components = Components::RGBA;
        result.Type = Type::SignedNormalized;
        result.RedBits = 16;
        result.GreenBits = 16;
        result.BlueBits = 16;
        result.AlphaBits = 16;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16G16B16A16_SINT:
        result.Components = Components::RGBA;
        result.Type = Type::SignedInteger;
        result.RedBits = 16;
        result.GreenBits = 16;
        result.BlueBits = 16;
        result.AlphaBits = 16;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R32G32_TYPELESS:
        result.Components = Components::RG;
        result.Type = Type::Typeless;
        result.RedBits = 32;
        result.GreenBits = 32;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R32G32_FLOAT:
        result.Components = Components::RG;
        result.Type = Type::Float;
        result.RedBits = 32;
        result.GreenBits = 32;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R32G32_UINT:
        result.Components = Components::RG;
        result.Type = Type::UnsignedInteger;
        result.RedBits = 32;
        result.GreenBits = 32;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R32G32_SINT:
        result.Components = Components::RG;
        result.Type = Type::SignedInteger;
        result.RedBits = 32;
        result.GreenBits = 32;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        result.Components = Components::DepthStencil;
        result.Type = Type::Float;
        result.RedBits = 0;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 32;
        result.StencilBits = 8;
        break;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        result.Components = Components::RGBA;
        result.Type = Type::Typeless;
        result.RedBits = 10;
        result.GreenBits = 10;
        result.BlueBits = 10;
        result.AlphaBits = 2;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
        result.Components = Components::RGBA;
        result.Type = Type::UnsignedNormalized;
        result.RedBits = 10;
        result.GreenBits = 10;
        result.BlueBits = 10;
        result.AlphaBits = 2;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R10G10B10A2_UINT:
        result.Components = Components::RGBA;
        result.Type = Type::UnsignedInteger;
        result.RedBits = 10;
        result.GreenBits = 10;
        result.BlueBits = 10;
        result.AlphaBits = 2;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R11G11B10_FLOAT:
        result.Components = Components::RGB;
        result.Type = Type::Float;
        result.RedBits = 11;
        result.GreenBits = 11;
        result.BlueBits = 10;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        result.Components = Components::RGBA;
        result.Type = Type::Typeless;
        result.RedBits = 8;
        result.GreenBits = 8;
        result.BlueBits = 8;
        result.AlphaBits = 8;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        result.Components = Components::RGBA;
        result.Type = Type::UnsignedNormalized;
        result.RedBits = 8;
        result.GreenBits = 8;
        result.BlueBits = 8;
        result.AlphaBits = 8;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R8G8B8A8_UINT:
        result.Components = Components::RGBA;
        result.Type = Type::UnsignedInteger;
        result.RedBits = 8;
        result.GreenBits = 8;
        result.BlueBits = 8;
        result.AlphaBits = 8;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R8G8B8A8_SNORM:
        result.Components = Components::RGBA;
        result.Type = Type::SignedNormalized;
        result.RedBits = 8;
        result.GreenBits = 8;
        result.BlueBits = 8;
        result.AlphaBits = 8;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R8G8B8A8_SINT:
        result.Components = Components::RGBA;
        result.Type = Type::SignedInteger;
        result.RedBits = 8;
        result.GreenBits = 8;
        result.BlueBits = 8;
        result.AlphaBits = 8;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16G16_TYPELESS:
        result.Components = Components::RG;
        result.Type = Type::Typeless;
        result.RedBits = 16;
        result.GreenBits = 16;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16G16_FLOAT:
        result.Components = Components::RG;
        result.Type = Type::Float;
        result.RedBits = 16;
        result.GreenBits = 16;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16G16_UNORM:
        result.Components = Components::RG;
        result.Type = Type::UnsignedNormalized;
        result.RedBits = 16;
        result.GreenBits = 16;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16G16_UINT:
        result.Components = Components::RG;
        result.Type = Type::UnsignedInteger;
        result.RedBits = 16;
        result.GreenBits = 16;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16G16_SINT:
        result.Components = Components::RG;
        result.Type = Type::SignedInteger;
        result.RedBits = 16;
        result.GreenBits = 16;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R32_TYPELESS:
        result.Components = Components::R;
        result.Type = Type::Typeless;
        result.RedBits = 32;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_D32_FLOAT:
        result.Components = Components::Depth;
        result.Type = Type::Float;
        result.RedBits = 0;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 32;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R32_FLOAT:
        result.Components = Components::R;
        result.Type = Type::Float;
        result.RedBits = 32;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R32_UINT:
        result.Components = Components::R;
        result.Type = Type::UnsignedInteger;
        result.RedBits = 32;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R32_SINT:
        result.Components = Components::R;
        result.Type = Type::SignedInteger;
        result.RedBits = 32;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R24G8_TYPELESS:
        result.Components = Components::RG;
        result.Type = Type::Typeless;
        result.RedBits = 24;
        result.GreenBits = 8;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        result.Components = Components::DepthStencil;
        result.Type = Type::UnsignedNormalized;
        result.RedBits = 0;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 24;
        result.StencilBits = 8;
        break;
    case DXGI_FORMAT_R8G8_TYPELESS:
        result.Components = Components::RG;
        result.Type = Type::Typeless;
        result.RedBits = 8;
        result.GreenBits = 8;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R8G8_UNORM:
        result.Components = Components::RG;
        result.Type = Type::UnsignedNormalized;
        result.RedBits = 8;
        result.GreenBits = 8;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R8G8_UINT:
        result.Components = Components::RG;
        result.Type = Type::UnsignedInteger;
        result.RedBits = 8;
        result.GreenBits = 8;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R8G8_SNORM:
        result.Components = Components::RG;
        result.Type = Type::SignedNormalized;
        result.RedBits = 8;
        result.GreenBits = 8;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R8G8_SINT:
        result.Components = Components::RG;
        result.Type = Type::SignedInteger;
        result.RedBits = 8;
        result.GreenBits = 8;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16_TYPELESS:
        result.Components = Components::R;
        result.Type = Type::Typeless;
        result.RedBits = 16;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16_FLOAT:
        result.Components = Components::R;
        result.Type = Type::Float;
        result.RedBits = 16;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_D16_UNORM:
        result.Components = Components::Depth;
        result.Type = Type::UnsignedNormalized;
        result.RedBits = 0;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 16;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16_UNORM:
        result.Components = Components::R;
        result.Type = Type::UnsignedNormalized;
        result.RedBits = 16;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16_UINT:
        result.Components = Components::R;
        result.Type = Type::UnsignedInteger;
        result.RedBits = 16;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16_SNORM:
        result.Components = Components::R;
        result.Type = Type::SignedNormalized;
        result.RedBits = 16;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R16_SINT:
        result.Components = Components::R;
        result.Type = Type::SignedInteger;
        result.RedBits = 16;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R8_TYPELESS:
        result.Components = Components::R;
        result.Type = Type::Typeless;
        result.RedBits = 8;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R8_UNORM:
        result.Components = Components::R;
        result.Type = Type::UnsignedNormalized;
        result.RedBits = 8;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R8_UINT:
        result.Components = Components::R;
        result.Type = Type::UnsignedInteger;
        result.RedBits = 8;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R8_SNORM:
        result.Components = Components::R;
        result.Type = Type::SignedNormalized;
        result.RedBits = 8;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R8_SINT:
        result.Components = Components::R;
        result.Type = Type::SignedInteger;
        result.RedBits = 8;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    case DXGI_FORMAT_R1_UNORM:
        result.Components = Components::R;
        result.Type = Type::UnsignedNormalized;
        result.RedBits = 1;
        result.GreenBits = 0;
        result.BlueBits = 0;
        result.AlphaBits = 0;
        result.DepthBits = 0;
        result.StencilBits = 0;
        break;
    default:
        ReportError( "Unsupported DXGI format." );
        break;
    }

    return result;
}


DXGI_SAMPLE_DESC TextureDX11::GetSupportedSampleCount( DXGI_FORMAT format, uint8_t numSamples )
{
    DXGI_SAMPLE_DESC sampleDesc;

    UINT sampleCount = 1;
    UINT qualityLevels = 0;

    while ( sampleCount <= numSamples && SUCCEEDED( m_pDevice->CheckMultisampleQualityLevels( format, sampleCount, &qualityLevels ) ) && qualityLevels > 0 )
    {
        // That works...
        sampleDesc.Count = sampleCount;
        sampleDesc.Quality = qualityLevels - 1;

        // But can we do better?
        sampleCount = sampleCount * 2;
    }

    return sampleDesc;
}

ID3D11Resource* TextureDX11::GetTextureResource() const
{
    ID3D11Resource* resource = nullptr;
    switch ( m_TextureDimension )
    {
    case Texture::Dimension::Texture1D:
    case Texture::Dimension::Texture1DArray:
        resource = m_pTexture1D.Get();
        break;
    case Texture::Dimension::Texture2D:
    case Texture::Dimension::Texture2DArray:
        resource = m_pTexture2D.Get();
        break;
    case Texture::Dimension::Texture3D:
    case Texture::Dimension::TextureCube:
        resource = m_pTexture3D.Get();
        break;
    }

    return resource;
}

ID3D11ShaderResourceView* TextureDX11::GetShaderResourceView() const
{
    return m_pShaderResourceView.Get();
}

ID3D11DepthStencilView* TextureDX11::GetDepthStencilView() const
{
    return m_pDepthStencilView.Get();
}

ID3D11RenderTargetView* TextureDX11::GetRenderTargetView() const
{
    return m_pRenderTargetView.Get();
}

ID3D11UnorderedAccessView* TextureDX11::GetUnorderedAccessView() const
{
    return m_pUnorderedAccessView.Get();
}

