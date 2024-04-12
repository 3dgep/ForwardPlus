
template< typename T >
void ConstantBuffer::Set( const T& data )
{
    Set( &data, sizeof( T ) );
}
