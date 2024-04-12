#include <EnginePCH.h>
#include <BoundingSphere.h>

inline float InvSqr( float v )
{
    return ( v != 0.0f ) ? 1.0f / ( v * v ) : 0.0f;
}

BoundingSphere::BoundingSphere( const glm::vec3& center, float radius )
    : m_Center( center )
    , m_Radius( radius )
    , m_InvRadiusSqr( 0 )
{
    m_InvRadiusSqr = InvSqr(m_Radius);
}

const glm::vec3& BoundingSphere::GetCenter() const
{
    return m_Center;
}

float BoundingSphere::GetRadius() const
{
    return m_Radius;
}

float BoundingSphere::GetInvRadiusSqr() const
{
    return m_InvRadiusSqr;
}

bool BoundingSphere::IsValid() const
{
    return m_Radius > 0.0f;
}

void BoundingSphere::Enlarge( const BoundingSphere& other )
{
    if ( !other.IsValid() ) return;

    if ( !IsValid() )
    {
        m_Center = other.m_Center;
        m_Radius = other.m_Radius;

        return;
    }

    // Compute the distance between the sphere centers.
    float distance = glm::distance( m_Center, other.m_Center );

    // The other sphere is completely inside this one.
    if ( distance + other.m_Radius <= m_Radius )
    {
        return;
    }

    // This sphere is completely inside the other one.
    if ( distance + m_Radius <= other.m_Radius )
    {
        m_Center = other.m_Center;
        m_Radius = other.m_Radius;

        return;
    }

    // Build a new sphere that completely contains both spheres.

    float newRadius = ( m_Radius + distance + other.m_Radius ) * 0.5f;
    float ratio = ( newRadius - m_Radius ) / distance;

    m_Center = ( other.m_Center - m_Center ) * ratio;
    m_Radius = newRadius;
}
