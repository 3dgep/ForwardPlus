
template< typename HandleType >
DescriptorHandleDX12<HandleType>::DescriptorHandleDX12( const DescriptorHandleDX12<HandleType>& copy )
    : m_StartOfHeap( copy.m_StartOfHeap )
    , m_CurrentLocation( copy.m_CurrentLocation )
    , m_Increment( copy.m_Increment )
{}

template< typename HandleType >
DescriptorHandleDX12<HandleType>::DescriptorHandleDX12( HandleType handle, UINT increment )
    : m_StartOfHeap( handle )
    , m_CurrentLocation( handle )
    , m_Increment( increment )
{}

template< typename HandleType >
DescriptorHandleDX12<HandleType>::operator HandleType() const
{
    return m_CurrentLocation;
}

template< typename HandleType >
DescriptorHandleDX12<HandleType>& DescriptorHandleDX12<HandleType>::operator++() // Pre-increment
{
    m_CurrentLocation.ptr += m_Increment;
    return *this;
}

template< typename HandleType >
DescriptorHandleDX12<HandleType> DescriptorHandleDX12<HandleType>::operator++( int ) // Post-increment
{
    DescriptorHandleDX12<HandleType> temp = *this;
    m_CurrentLocation.ptr += m_Increment;
    return temp;
}

template< typename HandleType >
DescriptorHandleDX12<HandleType>& DescriptorHandleDX12<HandleType>::operator--() // Pre-decrement operator.
{
    m_CurrentLocation.ptr -= m_Increment;
    return *this;
}

template< typename HandleType >
DescriptorHandleDX12<HandleType> DescriptorHandleDX12<HandleType>::operator--( int ) // Post-decrement operator.
{
    DescriptorHandleDX12<HandleType> temp = *this;
    m_CurrentLocation.ptr -= m_Increment;
    return temp;
}

template< typename HandleType >
DescriptorHandleDX12<HandleType>& DescriptorHandleDX12<HandleType>::operator+=( UINT count )
{
    m_CurrentLocation.ptr += ( m_Increment * count );
    return *this;
}

template< typename HandleType >
DescriptorHandleDX12<HandleType> DescriptorHandleDX12<HandleType>::operator+( UINT count ) const
{
    DescriptorHandleDX12<HandleType> temp = *this;
    temp.m_CurrentLocation.ptr += ( m_Increment * count );
    return temp;
}

template< typename HandleType >
DescriptorHandleDX12<HandleType>& DescriptorHandleDX12<HandleType>::operator-=( UINT count )
{
    m_CurrentLocation.ptr -= ( m_Increment * count );
    return *this;
}

template< typename HandleType >
DescriptorHandleDX12<HandleType> DescriptorHandleDX12<HandleType>::operator-( UINT count ) const
{
    DescriptorHandleDX12<HandleType> temp = *this;
    temp.m_CurrentLocation.ptr -= ( m_Increment * count );
    return temp;
}

template< typename HandleType >
DescriptorHandleDX12<HandleType>& DescriptorHandleDX12<HandleType>::operator=( UINT offset )
{
    m_CurrentLocation.ptr = m_StartOfHeap.ptr + ( m_Increment * offset );
    return *this;
}