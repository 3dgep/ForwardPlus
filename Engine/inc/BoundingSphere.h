#pragma once

class BoundingSphere
{
public:
    BoundingSphere( const glm::vec3& center = glm::vec3(0), float radius = 0 );

    const glm::vec3& GetCenter() const;
    float GetRadius() const;
    float GetInvRadiusSqr() const;

    // Valid if the radius is > 0.
    bool IsValid() const;

    /**
     * Enlarge this bounding sphere by adding another bounding sphere to it.
     */
    void Enlarge( const BoundingSphere& other );

private:
    glm::vec3   m_Center;
    float       m_Radius;
    float       m_InvRadiusSqr;    // 1 / ( radius^2)

};