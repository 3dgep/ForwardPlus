template< typename T >
void Texture::Plot( glm::ivec2 coord, const T& color )
{
    Plot( coord, reinterpret_cast<const uint8_t*>( &color ), sizeof( T ) );
}

template< typename T >
T Texture::FetchPixel( glm::ivec2 coord )
{
    uint8_t* pixel = nullptr;
    FetchPixel( coord, pixel, sizeof( T ) );

    return *reinterpret_cast<T*>( pixel );
}
