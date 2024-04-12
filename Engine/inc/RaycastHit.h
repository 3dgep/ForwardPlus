#pragma once
/**
 * RaycastHit structure is used to return the result of a Raycast
 */

class RaycastHit
{
public:
	// The point in 3D space where the ray hit the geometry.
	glm::vec3 Point;
	// The surface normal where the ray hit the geometry.
	glm::vec3 Normal;
	// The distance from the ray origin to the impact point.
	float Distance;

	// A pointer to the material that was hit (if one was, NULL otherwise)
	Material* pMaterial;

};