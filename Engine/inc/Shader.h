#pragma once
#include <Object.h>

class ShaderParameter;
class ConstantBuffer;

class Shader : public Object
{
public:
    typedef Object base;

    enum ShaderType
    {
		UnknownShaderType = 0,
        VertexShader,
        TessellationControlShader,      // Hull Shader in DirectX
        TessellationEvaluationShader,   // Domain Shader in DirectX
        GeometryShader,
        PixelShader,
        ComputeShader,
    };

    /**
     * What type of shader is this?
     */
    virtual ShaderType GetType() const = 0;

    /**
     * A shader macro consists of a macro name and a definition.
     * Use this to pass macro definitions to the shader compiler.
     */
    typedef std::map< std::string, std::string > ShaderMacros;


	/**
	 * Load a shader file from a string.
	 * @param type: The type of shader to load.
	 * @param source: The Shader source code in string format.
     * @param sourceFileName: The file path of the original file if it exists. This is used to determine include paths.
	 * @param entryPoint: The name of the entry-point function to be used by this shader.
	 * @param profile: The shader profile to use to compile this shader. 
	 * To use the latest supported profile, specify "latest" here.
	 * @return True if the shader was loaded correctly, or False otherwise.
	 */ 
	virtual bool LoadShaderFromString( ShaderType type, const std::string& source, const std::wstring& sourceFileName, const ShaderMacros& shaderMacros, const std::string& entryPoint, const std::string& profile ) = 0;

	/**
	 * Load a shader from a file.
	 * @param type: The type of shader to load.
	 * @param fileName: The path to the shader file to load.
	 * @param entryPoint: The name of the entry-point function to be used by this shader.
	 * @param profile: The shader profile to use to compile this shader. 
	 * To use the latest supported profile, specify "latest" here.
	 * @return True if the shader was loaded correctly, or False otherwise.
	 */ 
    virtual bool LoadShaderFromFile( ShaderType type, const std::wstring& fileName, const ShaderMacros& shaderMacros, const std::string& entryPoint, const std::string& profile ) = 0;

	/**
	 * Get a reference to a parameter defined in the shader.
	 * @param name: The name of the parameter as defined in the shader file.
	 * @return A reference to the ShaderParameter. If the parameter with the specified name
	 * is not found in the shader, then this function will return an invalid shader parameter.
	 * You can check for validity using the ShaderParameter::IsValid method.
	 */
    virtual ShaderParameter& GetShaderParameterByName( const std::string& name ) const = 0;
    // Get a parameter defined in the shader by its name by using in index operator.
    virtual ShaderParameter& operator[]( const std::string& name ) const
    {
        return GetShaderParameterByName( name );
    }

 //   /**
 //    * Gets a pointer to a constant buffer defined in the shader.
 //    */
 //   virtual ConstantBuffer* GetConstantBufferByName( const std::string& name ) = 0;
	
    /**
     * Gets the index (register slot) of a constant buffer defined in this shader.
     */
    //virtual UINT GetConstantBufferIndex( const std::string& name ) = 0;

	/**
	 * Query for the latest supported shader profile.
	 * @param type: The type of shader to query.
	 * @return The supported shader profile or an empty string if no profile could be 
	 * determined for the specified shader type.
	 */
	virtual std::string GetLatestProfile( ShaderType type ) = 0;

    /**
     * Bind this shader for use in the rendering pipeline.
     */
    virtual void Bind() = 0;

    /**
     * Unbind the shader from the rendering pipeline.
     */
    virtual void UnBind() = 0;

    /**
     * Dispatch a compute shader. If this shader does not refer to a compute 
     * shader, this function does nothing.
     * TODO: Refactor this into a Command (and CommandBuffer).
     */
    virtual void Dispatch( const glm::uvec3& numGroups ) = 0;
};