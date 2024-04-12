// Template parameter specializations for StructuredBuffer
template<typename T>
void StructuredBuffer::Set( const std::vector<T>& values )
{
    SetData( (void*)values.data(), sizeof(T), 0, values.size() );
}
