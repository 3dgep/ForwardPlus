#pragma once

#include <cstdint>

__declspec( align( 16 ) ) struct Light
{
    enum class LightType : uint32_t
    {
        Point = 0,
        Spot = 1,
        Directional = 2
    };

    /**
    * Position for point and spot lights (World space).
    */
    glm::vec4   m_PositionWS;
    //--------------------------------------------------------------( 16 bytes )
    /**
    * Direction for spot and directional lights (World space).
    */
    glm::vec4   m_DirectionWS;
    //--------------------------------------------------------------( 16 bytes )
    /**
    * Position for point and spot lights (View space).
    */
    glm::vec4   m_PositionVS;
    //--------------------------------------------------------------( 16 bytes )
    /**
    * Direction for spot and directional lights (View space).
    */
    glm::vec4   m_DirectionVS;
    //--------------------------------------------------------------( 16 bytes )
    /**
     * Color of the light. Diffuse and specular colors are not separated.
     */
    glm::vec4   m_Color;
    //--------------------------------------------------------------( 16 bytes )
    /**
     * The half angle of the spotlight cone.
     */
    float       m_SpotlightAngle;
    /**
     * The range of the light.
     */
    float       m_Range;

    /**
     * The intensity of the light.
     */
    float       m_Intensity;

    /**
     * Disable or enable the light.
     */
    uint32_t    m_Enabled;
    //--------------------------------------------------------------(16 bytes )
    
    /**
     * True if the light is selected in the editor.
     */
    uint32_t    m_Selected;
    /**
     * The type of the light. 
     */
    LightType   m_Type;

    glm::vec2   m_Padding;
    //--------------------------------------------------------------(16 bytes )
    //--------------------------------------------------------------( 16 * 7 = 112 bytes )
    Light::Light()
        : m_PositionWS( 0, 0, 0, 1 )
        , m_DirectionWS( 0, 0, -1, 0 )
        , m_PositionVS( 0, 0, 0, 1 )
        , m_DirectionVS( 0, 0, 1, 0 )
        , m_Color( 1, 1, 1, 1 )
        , m_SpotlightAngle( 45.0f )
        , m_Range( 100.0f )
        , m_Intensity( 1.0f )
        , m_Enabled( true )
        , m_Selected( false )
        , m_Type( LightType::Point )
    {}
};