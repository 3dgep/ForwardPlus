// A Ray structure that can be used in raycasting renderers.
#pragma once

class Material;

class Ray
{
public:
    Ray();
	Ray( glm::vec3 origin, glm::vec3 direction );

	// Gets a point that is distance units along the ray.
	glm::vec3 GetPointOnRay( float distance ) const;

	// The origin of the ray in 3D space.
	glm::vec3 m_Origin;
	// The normalized direction of the ray in 3D space.
	glm::vec3 m_Direction;

	// The material that was hit.
	Material* m_pMaterial;
};