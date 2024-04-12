template<>
void ShaderParameter::Set<ConstantBuffer>( std::shared_ptr<ConstantBuffer> value );

template<>
void ShaderParameter::Set<Texture>( std::shared_ptr<Texture> value );

template<>
void ShaderParameter::Set<SamplerState>( std::shared_ptr<SamplerState> value );

template<>
void ShaderParameter::Set<StructuredBuffer>( std::shared_ptr<StructuredBuffer> value );

template<typename T>
void ShaderParameter::Set( std::shared_ptr<T> value )
{
    BOOST_STATIC_ASSERT_MSG( false, "This function must be specialized.");
}