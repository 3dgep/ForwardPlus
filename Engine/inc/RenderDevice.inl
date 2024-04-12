// Template specializations for vertex buffers.
template<>
std::shared_ptr<Buffer> RenderDevice::CreateVertexBuffer< std::vector<float> >( const std::vector<float>& data );
template<>
std::shared_ptr<Buffer> RenderDevice::CreateVertexBuffer< std::vector<glm::vec2> >( const std::vector<glm::vec2>& data );
template<>
std::shared_ptr<Buffer> RenderDevice::CreateVertexBuffer< std::vector<glm::vec3> >( const std::vector<glm::vec3>& data );
template<>
std::shared_ptr<Buffer> RenderDevice::CreateVertexBuffer< std::vector<glm::vec4> >( const std::vector<glm::vec4>& data );
//template<>
//const Buffer* RenderDevice::CreateVertexBuffer< std::vector< aiVector3D > >( const std::vector< aiVector3D >& data );

// Template specializations for index buffers.
template<>
std::shared_ptr<Buffer> RenderDevice::CreateIndexBuffer< std::vector<unsigned int> >( const std::vector<unsigned int>& data );

// Non-specialized template methods.
template< typename T >
std::shared_ptr<Buffer> RenderDevice::CreateVertexBuffer( const T& data )
{
    BOOST_STATIC_ASSERT_MSG( false, "This function must be specialized." );
    return NULL;
}

template<typename T>
std::shared_ptr<Buffer> RenderDevice::CreateIndexBuffer( const T& data )
{
    BOOST_STATIC_ASSERT_MSG( false, "This function must be specialized." );
    return NULL;
}

template< typename T >
std::shared_ptr<ConstantBuffer> RenderDevice::CreateConstantBuffer( const T& data )
{
    return CreateConstantBuffer( &data, sizeof( T ) );
}

template<typename T>
std::shared_ptr<StructuredBuffer> RenderDevice::CreateStructuredBuffer( const std::vector<T>& data, CPUAccess cpuAccess, bool gpuWrite )
{
    size_t stride = sizeof( T );
    size_t numElements = data.size();
    return CreateStructuredBuffer( (void*)data.data(), (unsigned int)numElements, (unsigned int)stride, cpuAccess, gpuWrite );
}