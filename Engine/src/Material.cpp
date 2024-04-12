#include <EnginePCH.h>

#include <RenderDevice.h>
#include <Shader.h>
#include <ShaderParameter.h>
#include <Texture.h>
#include <ConstantBuffer.h>

#include <Material.h>

Material::Material( RenderDevice& renderDevice )
    : m_RenderDevice( renderDevice )
    , m_Dirty( false )
{
    m_pProperties = (MaterialProperties*)_aligned_malloc( sizeof( MaterialProperties ), 16 );
    // Construct default material properties.
    *m_pProperties = MaterialProperties();

    // Initialize the constant buffer.
    m_pConstantBuffer = m_RenderDevice.CreateConstantBuffer( *m_pProperties );
}

Material::~Material()
{
    if ( m_pConstantBuffer )
    {
        m_RenderDevice.DestroyConstantBuffer( m_pConstantBuffer );
        m_pConstantBuffer.reset();
    }

    if ( m_pProperties) 
    {
        _aligned_free( m_pProperties );
        m_pProperties = nullptr;
    }
}

void Material::Bind( std::weak_ptr<Shader> wpShader ) const
{
    std::shared_ptr<Shader> pShader = wpShader.lock();
    if ( !pShader ) return;

    if ( m_Dirty )
    {
        // Make sure the constant buffer associated to this material is updated.
        Material* _this = const_cast<Material*>( this );
        _this->UpdateConstantBuffer();
        _this->m_Dirty = false;
    }

    // OOPS.. Dangerous. Just blindly set all textures associated to this material.
    // Maybe I should check the names of the textures in the shader before doing this?
    // I could be replacing textures that are bound to the shader that shouldn't be changed!?
    // (Because they have been specified by the user for example).
    for ( auto texture : m_Textures )
    {
        std::shared_ptr<Texture> pTexture = texture.second;
        pTexture->Bind( (uint32_t)texture.first, pShader->GetType(), ShaderParameter::Type::Texture );
    }

    // If the shader has a parameter called "Material".
    ShaderParameter& materialParameter = pShader->GetShaderParameterByName( "Material" );
    if ( materialParameter.IsValid() )
    {
        // Assign this material's constant buffer to it.
        materialParameter.Set<ConstantBuffer>( m_pConstantBuffer );
        // If the shader parameter is modified, they have to be 
        // rebound to update the rendering pipeline.
        materialParameter.Bind();
    }
}

const glm::vec4& Material::GetGlobalAmbientColor() const
{
    return m_pProperties->m_GlobalAmbient;
}

void Material::SetGlobalAmbientColor( const glm::vec4& globalAmbient )
{
    m_pProperties->m_GlobalAmbient = globalAmbient;
    m_Dirty = true;
}

const glm::vec4& Material::GetAmbientColor() const
{
    return m_pProperties->m_AmbientColor;
}

void Material::SetAmbientColor( const glm::vec4& ambient )
{
    m_pProperties->m_AmbientColor = ambient;
    m_Dirty = true;
}

const glm::vec4& Material::GetDiffuseColor() const
{
	return m_pProperties->m_DiffuseColor;
}

void Material::SetDiffuseColor( const glm::vec4& diffuse )
{
    m_pProperties->m_DiffuseColor = diffuse;
    m_Dirty = true;
}

const glm::vec4& Material::GetEmissiveColor() const
{
	return m_pProperties->m_EmissiveColor;
}

void Material::SetEmissiveColor( const glm::vec4& emissive )
{
    m_pProperties->m_EmissiveColor = emissive;
    m_Dirty = true;
}

const glm::vec4& Material::GetSpecularColor() const
{
	return m_pProperties->m_SpecularColor;
}

void Material::SetSpecularColor( const glm::vec4& specular )
{
    m_pProperties->m_SpecularColor = specular;
    m_Dirty = true;
}

float Material::GetSpecularPower() const
{
	return m_pProperties->m_SpecularPower;
}

const float Material::GetOpacity() const
{
    return m_pProperties->m_Opacity;
}

void Material::SetOpacity( float Opacity )
{
    m_pProperties->m_Opacity = Opacity;
    m_Dirty = true;
}

void Material::SetSpecularPower( float phongPower )
{
    m_pProperties->m_SpecularPower = phongPower;
    m_Dirty = true;
}

const glm::vec4& Material::GetReflectance() const
{
    return m_pProperties->m_Reflectance;
}

void Material::SetReflectance( const glm::vec4& reflectance )
{
    m_pProperties->m_Reflectance = reflectance;
    m_Dirty = true;
}

float Material::GetIndexOfRefraction() const
{
    return m_pProperties->m_IndexOfRefraction;
}

void Material::SetIndexOfRefraction( float indexOfRefraction )
{
    m_pProperties->m_IndexOfRefraction = indexOfRefraction;
    m_Dirty = true;
}

float Material::GetBumpIntensity() const
{
    return m_pProperties->m_BumpIntensity;
}
void Material::SetBumpIntensity( float bumpIntensity )
{
    m_pProperties->m_BumpIntensity = bumpIntensity;
}

std::shared_ptr<Texture> Material::GetTexture( TextureType type ) const
{
    TextureMap::const_iterator itr = m_Textures.find( type );
    if ( itr != m_Textures.end() )
    {
        return itr->second;
    }

    return nullptr;
}

void Material::SetTexture( TextureType type, std::shared_ptr<Texture> texture )
{
    m_Textures[type] = texture;

    switch ( type )
    {
    case TextureType::Ambient:
    {
        m_pProperties->m_HasAmbientTexture = ( texture != nullptr );
    }
    break;
    case TextureType::Emissive:
    {
        m_pProperties->m_HasEmissiveTexture = ( texture != nullptr );
    }
    break;
    case TextureType::Diffuse:
    {
        m_pProperties->m_HasDiffuseTexture = ( texture != nullptr );
    }
    break;
    case TextureType::Specular:
    {
        m_pProperties->m_HasSpecularTexture = ( texture != nullptr );
    }
    break;
    case TextureType::SpecularPower:
    {
        m_pProperties->m_HasSpecularPowerTexture = ( texture != nullptr );
    }
    break;
    case TextureType::Normal:
    {
        m_pProperties->m_HasNormalTexture = ( texture != nullptr );
    }
    break;
    case TextureType::Bump:
    {
        m_pProperties->m_HasBumpTexture = ( texture != nullptr );
    }
    break;
    case TextureType::Opacity:
    {
        m_pProperties->m_HasOpacityTexture = ( texture != nullptr );
    }
    break;
    }

    m_Dirty = true;
}

bool Material::IsTransparent() const
{
    return ( m_pProperties->m_Opacity < 1.0f ||
             m_pProperties->m_HasOpacityTexture ||
             ( m_pProperties->m_HasDiffuseTexture && GetTexture( TextureType::Diffuse )->IsTransparent() ) || 
             m_pProperties->m_AlphaThreshold <= 0.0f ); // Objects with an alpha threshold > 0 should be drawn in the opaque pass.
}

void Material::UpdateConstantBuffer()
{
    if ( m_pConstantBuffer )
    {
        m_pConstantBuffer->Set( *m_pProperties );
    }
}
