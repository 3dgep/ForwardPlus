// Some parts of this code have been inspired by:
// http://www.rastertek.com/dx11tut04.html
// Accessed Feb 2013.

#include <EnginePCH.h>
#include <Timer.h>

#include "ShaderParameterDX11.h"
#include "ShaderDX11.h"

// This parameter will be returned if an invalid shader parameter is requested.
static ShaderParameterDX11 gs_InvalidShaderParameter;

// Forward declarations
DXGI_FORMAT GetDXGIFormat( const D3D11_SIGNATURE_PARAMETER_DESC& paramDesc );

ShaderDX11::ShaderDX11( ID3D11Device2* pDevice )
    : m_ShaderType( UnknownShaderType )
    , m_pDevice( pDevice )
    , m_bFileChanged ( false )
{
    m_pDevice->GetImmediateContext2( &m_pDeviceContext );

    m_Connections.push_back( m_DependencyTracker.FileChanged += boost::bind( &ShaderDX11::OnFileChanged, this, _1 ) );
}

ShaderDX11::~ShaderDX11()
{
    Destroy();
}

void ShaderDX11::Destroy()
{
    m_pPixelShader.Reset();
    m_pDomainShader.Reset();
    m_pHullShader.Reset();
    m_pGeometryShader.Reset();
    m_pVertexShader.Reset();
    m_pComputeShader.Reset();
    m_pInputLayout.Reset();

    m_ShaderParameters.clear();
    m_InputSemantics.clear();
}

Shader::ShaderType ShaderDX11::GetType() const
{
    return m_ShaderType;
}


std::string ShaderDX11::GetLatestProfile( ShaderType type )
{
    // Query the current feature level:
    D3D_FEATURE_LEVEL featureLevel = m_pDevice->GetFeatureLevel();

    switch ( type )
    {
    case VertexShader:
        switch ( featureLevel )
        {
        case D3D_FEATURE_LEVEL_11_1:
        case D3D_FEATURE_LEVEL_11_0:
            return "vs_5_0";
            break;
        case D3D_FEATURE_LEVEL_10_1:
            return "vs_4_1";
            break;
        case D3D_FEATURE_LEVEL_10_0:
            return "vs_4_0";
            break;
        case D3D_FEATURE_LEVEL_9_3:
            return "vs_4_0_level_9_3";
            break;
        case D3D_FEATURE_LEVEL_9_2:
        case D3D_FEATURE_LEVEL_9_1:
            return "vs_4_0_level_9_1";
            break;
        }
        break;
    case TessellationControlShader:
        switch ( featureLevel )
        {
        case D3D_FEATURE_LEVEL_11_1:
        case D3D_FEATURE_LEVEL_11_0:
            return "ds_5_0";
            break;
        }
        break;
    case TessellationEvaluationShader:
        switch ( featureLevel )
        {
        case D3D_FEATURE_LEVEL_11_1:
        case D3D_FEATURE_LEVEL_11_0:
            return "hs_5_0";
            break;
        }
        break;
    case GeometryShader:
        switch ( featureLevel )
        {
        case D3D_FEATURE_LEVEL_11_1:
        case D3D_FEATURE_LEVEL_11_0:
            return "gs_5_0";
            break;
        case D3D_FEATURE_LEVEL_10_1:
            return "gs_4_1";
            break;
        case D3D_FEATURE_LEVEL_10_0:
            return "gs_4_0";
            break;
        }
        break;
    case PixelShader:
        switch ( featureLevel )
        {
        case D3D_FEATURE_LEVEL_11_1:
        case D3D_FEATURE_LEVEL_11_0:
            return "ps_5_0";
            break;
        case D3D_FEATURE_LEVEL_10_1:
            return "ps_4_1";
            break;
        case D3D_FEATURE_LEVEL_10_0:
            return "ps_4_0";
            break;
        case D3D_FEATURE_LEVEL_9_3:
            return "ps_4_0_level_9_3";
            break;
        case D3D_FEATURE_LEVEL_9_2:
        case D3D_FEATURE_LEVEL_9_1:
            return "ps_4_0_level_9_1";
            break;
        }
        break;
    case ComputeShader:
        switch ( featureLevel )
        {
        case D3D_FEATURE_LEVEL_11_1:
        case D3D_FEATURE_LEVEL_11_0:
            return "cs_5_0";
            break;
        case D3D_FEATURE_LEVEL_10_1:
            return "cs_4_1";
            break;
        case D3D_FEATURE_LEVEL_10_0:
            return "cs_4_0";
            break;
        }
    } // switch( type )

    // Throw an exception?
    return "";
}
bool ShaderDX11::LoadShaderFromString( ShaderType shaderType, const std::string& source, const std::wstring& sourceFileName, const ShaderMacros& shaderMacros, const std::string& entryPoint, const std::string& profile )
{
    HRESULT hr;
    {
        Microsoft::WRL::ComPtr<ID3DBlob> pShaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob;

        std::string _profile = profile;
        if ( profile == "latest" )
        {
            _profile = GetLatestProfile( shaderType );
            if ( _profile.empty() )
            {
                ReportError( "Invalid shader type for feature level." );
                return false;
            }
        }

        std::vector<D3D_SHADER_MACRO> macros;
        for ( auto macro : shaderMacros )
        {
            // The macro definitions passed to this function only store temporary string objects.
            // I need to copy the temporary strings into the D3D macro type 
            // in order for it to persist outside of this for loop.
            std::string name = macro.first;
            std::string definition = macro.second;

            char* c_name = new char[name.size() + 1];
            char* c_definition = new char[definition.size() + 1];

            strncpy_s( c_name, name.size() + 1, name.c_str(), name.size() );
            strncpy_s( c_definition, definition.size() + 1, definition.c_str(), definition.size() );

            macros.push_back( { c_name, c_definition } );
        }
        macros.push_back( { 0, 0 } );


        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( _DEBUG )
        flags |= D3DCOMPILE_DEBUG;
#endif

        fs::path filePath( sourceFileName );
        std::string sourceFilePath = filePath.string();

        hr = D3DCompile( (LPCVOID)source.c_str(), source.size(), sourceFilePath.c_str(), macros.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), _profile.c_str(), flags, 0, &pShaderBlob, &pErrorBlob );

        // We're done compiling.. Delete the macro definitions.
        for ( D3D_SHADER_MACRO macro : macros )
        {
            delete [] macro.Name;
            delete [] macro.Definition;
        }
        macros.clear();

        if ( FAILED( hr ) )
        {
            if ( pErrorBlob )
            {
                OutputDebugStringA( static_cast<char*>( pErrorBlob->GetBufferPointer() ) );
                ReportError( static_cast<char*>( pErrorBlob->GetBufferPointer() ) );
            }
            return false;
        }

        m_pShaderBlob = pShaderBlob;
    }

    // After the shader recompiles, try to restore the shader parameters.
    ParameterMap shaderParameters = m_ShaderParameters;

    // Destroy the last shader as we are now loading a new one.
    Destroy();

    m_ShaderType = shaderType;

    switch ( m_ShaderType )
    {
    case VertexShader:
        hr = m_pDevice->CreateVertexShader( m_pShaderBlob->GetBufferPointer(), m_pShaderBlob->GetBufferSize(), nullptr, &m_pVertexShader );
        break;
    case TessellationControlShader:
        hr = m_pDevice->CreateHullShader( m_pShaderBlob->GetBufferPointer(), m_pShaderBlob->GetBufferSize(), nullptr, &m_pHullShader );
        break;
    case TessellationEvaluationShader:
        hr = m_pDevice->CreateDomainShader( m_pShaderBlob->GetBufferPointer(), m_pShaderBlob->GetBufferSize(), nullptr, &m_pDomainShader );
        break;
    case GeometryShader:
        hr = m_pDevice->CreateGeometryShader( m_pShaderBlob->GetBufferPointer(), m_pShaderBlob->GetBufferSize(), nullptr, &m_pGeometryShader );
        break;
    case PixelShader:
        hr = m_pDevice->CreatePixelShader( m_pShaderBlob->GetBufferPointer(), m_pShaderBlob->GetBufferSize(), nullptr, &m_pPixelShader );
        break;
    case ComputeShader:
        hr = m_pDevice->CreateComputeShader( m_pShaderBlob->GetBufferPointer(), m_pShaderBlob->GetBufferSize(), nullptr, &m_pComputeShader );
        break;
    default:
        ReportError( "Invalid shader type." );
        break;
    }

    if ( FAILED( hr ) )
    {
        ReportError( "Failed to create shader." );
        return false;
    }

    // Reflect the parameters from the shader.
    // Inspired by: http://members.gamedev.net/JasonZ/Heiroglyph/D3D11ShaderReflection.pdf
    Microsoft::WRL::ComPtr<ID3D11ShaderReflection> pReflector;
    hr = D3DReflect( m_pShaderBlob->GetBufferPointer(), m_pShaderBlob->GetBufferSize(), IID_ID3D11ShaderReflection, &pReflector );

    if ( FAILED( hr ) )
    {
        ReportError( "Failed to reflect shader." );
        return false;
    }

    // Query input parameters and build the input layout
    D3D11_SHADER_DESC shaderDescription;
    hr = pReflector->GetDesc( &shaderDescription );

    if ( FAILED( hr ) )
    {
        ReportError( "Failed to get shader description from shader reflector." );
        return false;
    }

    m_InputSemantics.clear();

    UINT numInputParameters = shaderDescription.InputParameters;
    std::vector<D3D11_INPUT_ELEMENT_DESC> inputElements;
    for ( UINT i = 0; i < numInputParameters; ++i )
    {
        D3D11_INPUT_ELEMENT_DESC inputElement;
        D3D11_SIGNATURE_PARAMETER_DESC parameterSignature;

        pReflector->GetInputParameterDesc( i, &parameterSignature );

        inputElement.SemanticName = parameterSignature.SemanticName;
        inputElement.SemanticIndex = parameterSignature.SemanticIndex;
        inputElement.InputSlot = i; // TODO: If using interleaved arrays, then the input slot should be 0.  If using packed arrays, the input slot will vary.
        inputElement.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        inputElement.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA; // TODO: Figure out how to deal with per-instance data? .. Don't. Just use structured buffers to store per-instance data and use the SV_InstanceID as an index in the structured buffer.
        inputElement.InstanceDataStepRate = 0;
        inputElement.Format = GetDXGIFormat( parameterSignature );

        assert( inputElement.Format != DXGI_FORMAT_UNKNOWN );

        inputElements.push_back( inputElement );

        m_InputSemantics.insert( SemanticMap::value_type( BufferBinding( inputElement.SemanticName, inputElement.SemanticIndex ), i ) );
    }

    if ( inputElements.size() > 0 )
    {
        hr = m_pDevice->CreateInputLayout( inputElements.data(), (UINT)inputElements.size(), m_pShaderBlob->GetBufferPointer(), m_pShaderBlob->GetBufferSize(), &m_pInputLayout );
        if ( FAILED( hr ) )
        {
            ReportError( "Failed to create input layout." );
            return false;
        }
    }

    // Query Resources that are bound to the shader.
    for ( UINT i = 0; i < shaderDescription.BoundResources; ++i )
    {
        D3D11_SHADER_INPUT_BIND_DESC bindDesc;
        pReflector->GetResourceBindingDesc( i, &bindDesc );
        std::string resourceName = bindDesc.Name;

        ShaderParameter::Type parameterType = ShaderParameter::Type::Invalid;

        switch ( bindDesc.Type )
        {
        case D3D_SIT_TEXTURE:
            parameterType = ShaderParameter::Type::Texture;
            break;
        case D3D_SIT_SAMPLER:
            parameterType = ShaderParameter::Type::Sampler;
            break;
        case D3D_SIT_CBUFFER:
        case D3D_SIT_STRUCTURED:
            parameterType = ShaderParameter::Type::Buffer;
            break;
        case D3D_SIT_UAV_RWSTRUCTURED:
            parameterType = ShaderParameter::Type::RWBuffer;
            break;
        case D3D_SIT_UAV_RWTYPED:
            parameterType = ShaderParameter::Type::RWTexture;
            break;
        }

        // Create an empty shader parameter that should be filled-in by the application.
        std::shared_ptr<ShaderParameterDX11> shaderParameter = std::make_shared<ShaderParameterDX11>( resourceName, bindDesc.BindPoint, shaderType, parameterType );
        m_ShaderParameters.insert( ParameterMap::value_type( resourceName, shaderParameter ) );

    }

    // Now try to restore the original shader parameters (if there were any)
    for ( auto shaderParameter : shaderParameters )
    {
        ParameterMap::iterator iter = m_ShaderParameters.find( shaderParameter.first );
        if ( iter != m_ShaderParameters.end() )
        {
            iter->second = shaderParameter.second;
        }
    }

    return true;
}

bool ShaderDX11::LoadShaderFromFile( ShaderType shaderType, const std::wstring& fileName, const ShaderMacros& shaderMacros, const std::string& entryPoint, const std::string& profile )
{
    bool result = false;

    fs::path filePath( fileName );
    if ( fs::exists( filePath ) && fs::is_regular_file( filePath ) )
    {
        // Store data necessary to reload the shader if it changes on disc.
        m_ShaderFileName = fileName;
        m_ShaderMacros = shaderMacros;
        m_EntryPoint = entryPoint;
        m_Profile = profile;

        // Create a dependency tracker so we can reload the shader if either the 
        // shader file or any of it's dependencies have changed on disk.
        m_DependencyTracker = DependencyTracker( fileName );
        if ( !m_DependencyTracker.Load() )
        {
            // Save the configuration file for the dependency tracker (if it doesn't exist 
            // for the loaded asset).
            m_DependencyTracker.Save();
        }

        // Update the last load time (even if shader compilation fails).
        // Not doing this will result in the shaders being reloaded every frame
        // even if shader compilation fails.
        m_DependencyTracker.SetLastLoadTime();

        std::ifstream inputFile( fileName );
        std::string source( ( std::istreambuf_iterator<char>( inputFile ) ), std::istreambuf_iterator<char>() );

        result = LoadShaderFromString( shaderType, source, fileName, shaderMacros, entryPoint, profile );
    }
    else
    {
        ReportError( "Failed to load shader." );
    }

    return result;
}

ShaderParameter& ShaderDX11::GetShaderParameterByName( const std::string& name ) const
{
    ParameterMap::const_iterator iter = m_ShaderParameters.find( name );
    if ( iter != m_ShaderParameters.end() )
    {
        return *( iter->second );
    }

    return gs_InvalidShaderParameter;
}

bool ShaderDX11::HasSemantic( const BufferBinding& binding ) const
{
    SemanticMap::const_iterator iter = m_InputSemantics.find( binding );
    return iter != m_InputSemantics.end();
}

UINT ShaderDX11::GetSlotIDBySemantic( const BufferBinding& binding ) const
{
    SemanticMap::const_iterator iter = m_InputSemantics.find( binding );
    if ( iter != m_InputSemantics.end() )
    {
        return iter->second;
    }

    // Some kind of error code or exception...
    return (UINT)-1;
}

void ShaderDX11::OnFileChanged( FileChangeEventArgs& e )
{
    m_bFileChanged = true;
}

void ShaderDX11::Bind()
{
    if ( m_bFileChanged && m_DependencyTracker.IsStale() )
    {
        MutexLock lock( m_Mutex );
        // Check to see if the shader or any of it's dependencies have changed on disk
        // and reload the shader if so.
        LoadShaderFromFile( m_ShaderType, m_ShaderFileName, m_ShaderMacros, m_EntryPoint, m_Profile );
        m_bFileChanged = false;
    }

    for ( ParameterMap::value_type value : m_ShaderParameters )
    {
        value.second->Bind();
    }

    if ( m_pVertexShader )
    {
        m_pDeviceContext->IASetInputLayout( m_pInputLayout.Get() );
        m_pDeviceContext->VSSetShader( m_pVertexShader.Get(), nullptr, 0 );
    }
    else if ( m_pHullShader )
    {
        m_pDeviceContext->HSSetShader( m_pHullShader.Get(), nullptr, 0 );
    }
    else if ( m_pDomainShader )
    {
        m_pDeviceContext->DSSetShader( m_pDomainShader.Get(), nullptr, 0 );
    }
    else if ( m_pGeometryShader )
    {
        m_pDeviceContext->GSSetShader( m_pGeometryShader.Get(), nullptr, 0 );
    }
    else if ( m_pPixelShader )
    {
        m_pDeviceContext->PSSetShader( m_pPixelShader.Get(), nullptr, 0 );
    }
    else if ( m_pComputeShader )
    {
        m_pDeviceContext->CSSetShader( m_pComputeShader.Get(), nullptr, 0 );
    }
}

void ShaderDX11::UnBind()
{
    for ( ParameterMap::value_type value : m_ShaderParameters )
    {
        value.second->UnBind();
    }

    if ( m_pVertexShader )
    {
        m_pDeviceContext->IASetInputLayout( nullptr );
        m_pDeviceContext->VSSetShader( nullptr, nullptr, 0 );
    }
    else if ( m_pHullShader )
    {
        m_pDeviceContext->HSSetShader( nullptr, nullptr, 0 );
    }
    else if ( m_pDomainShader )
    {
        m_pDeviceContext->DSSetShader( nullptr, nullptr, 0 );
    }
    else if ( m_pGeometryShader )
    {
        m_pDeviceContext->GSSetShader( nullptr, nullptr, 0 );
    }
    else if ( m_pPixelShader )
    {
        m_pDeviceContext->PSSetShader( nullptr, nullptr, 0 );
    }
    else if ( m_pComputeShader )
    {
        m_pDeviceContext->CSSetShader( nullptr, nullptr, 0 );
    }
}

void ShaderDX11::Dispatch( const glm::uvec3& numGroups )
{
    if ( m_pDeviceContext && m_pComputeShader )
    {
        m_pDeviceContext->Dispatch( numGroups.x, numGroups.y, numGroups.z );
    }
}


// Determine DXGI format
// Inspired by: http://takinginitiative.net/2011/12/11/directx-1011-basic-shader-reflection-automatic-input-layout-creation/
DXGI_FORMAT GetDXGIFormat( const D3D11_SIGNATURE_PARAMETER_DESC& paramDesc )
{
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    if ( paramDesc.Mask == 1 ) // 1 component
    {
        switch ( paramDesc.ComponentType )
        {
        case D3D_REGISTER_COMPONENT_UINT32:
        {
            format = DXGI_FORMAT_R32_UINT;
        }
        break;
        case D3D_REGISTER_COMPONENT_SINT32:
        {
            format = DXGI_FORMAT_R32_SINT;
        }
        break;
        case D3D_REGISTER_COMPONENT_FLOAT32:
        {
            format = DXGI_FORMAT_R32_FLOAT;
        }
        break;
        }
    }
    else if ( paramDesc.Mask <= 3 ) // 2 components
    {
        switch ( paramDesc.ComponentType )
        {
        case D3D_REGISTER_COMPONENT_UINT32:
        {
            format = DXGI_FORMAT_R32G32_UINT;
        }
        break;
        case D3D_REGISTER_COMPONENT_SINT32:
        {
            format = DXGI_FORMAT_R32G32_SINT;
        }
        break;
        case D3D_REGISTER_COMPONENT_FLOAT32:
        {
            format = DXGI_FORMAT_R32G32_FLOAT;
        }
        break;
        }
    }
    else if ( paramDesc.Mask <= 7 ) // 3 components
    {
        switch ( paramDesc.ComponentType )
        {
        case D3D_REGISTER_COMPONENT_UINT32:
        {
            format = DXGI_FORMAT_R32G32B32_UINT;
        }
        break;
        case D3D_REGISTER_COMPONENT_SINT32:
        {
            format = DXGI_FORMAT_R32G32B32_SINT;
        }
        break;
        case D3D_REGISTER_COMPONENT_FLOAT32:
        {
            format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        break;
        }
    }
    else if ( paramDesc.Mask <= 15 ) // 4 components
    {
        switch ( paramDesc.ComponentType )
        {
        case D3D_REGISTER_COMPONENT_UINT32:
        {
            format = DXGI_FORMAT_R32G32B32A32_UINT;
        }
        break;
        case D3D_REGISTER_COMPONENT_SINT32:
        {
            format = DXGI_FORMAT_R32G32B32A32_SINT;
        }
        break;
        case D3D_REGISTER_COMPONENT_FLOAT32:
        {
            format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        break;
        }
    }

    return format;
}

