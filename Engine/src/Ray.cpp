#include <EnginePCH.h>
#include <Ray.h>

Ray::Ray()
{}

Ray::Ray( glm::vec3 origin, glm::vec3 direction )
	: m_Origin( origin )
	, m_Direction( direction )
{}

glm::vec3 Ray::GetPointOnRay( float distance ) const
{
	return m_Origin + ( m_Direction * distance );
}