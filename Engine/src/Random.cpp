#include <EnginePCH.h>

#include <Random.h>

Random::Random( int seed )
	: m_RandomGenerator(seed)
{}

int32_t Random::NextInt()
{
	return m_IntDistribution(m_RandomGenerator);
}

uint32_t Random::NextUInt()
{
	return m_UIntDistribution(m_RandomGenerator);
}

float Random::NextFloat()
{
	return m_FloatDistribution(m_RandomGenerator);
}

float Random::Range( float min, float max )
{
    return NextFloat() * ( max - min ) + min;
}

glm::vec2 Random::NextVec2f()
{
	return glm::vec2( NextFloat(), NextFloat() );
}

glm::vec3 Random::NextVec3f()
{
	return glm::vec3( NextFloat(), NextFloat(), NextFloat() );
}

glm::vec2 Random::UnitVector2f()
{
    const float angle = NextFloat() * 2.0f * glm::pi<float>();
    return glm::vec2( std::cos(angle), std::sin(angle) );
}

glm::vec3 Random::UnitVector3f()
{
    const float z = Range(-1,1);
    const glm::vec2 disc = UnitVector2f() * std::sqrt( 1.0f - (z*z) );
    return glm::vec3( disc.x, disc.y, z );
}

glm::vec3 Random::Hemisphere( const glm::vec3& normal )
{
    // 1. Generate a random unit vector in a sphere
    glm::vec3 unitVector = UnitVector3f();
    // Negate it if it is pointing away from the normal
    if ( glm::dot(unitVector,normal) < 0.0f )
    {
        unitVector = -unitVector;
    }

    return unitVector;
}