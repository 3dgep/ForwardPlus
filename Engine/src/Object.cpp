#include <EnginePCH.h>
#include <Object.h>

Object::Object()
    : m_UUID( boost::uuids::random_generator()() )
{}

Object::Object( ConstructorType type )
{

}

Object::~Object()
{}

bool Object::operator==( const Object& rhs ) const
{
    return m_UUID == rhs.m_UUID;
}

boost::uuids::uuid Object::GetUUID() const
{
    return m_UUID;
}
