#pragma once

#include <Light.h>

enum class LightGeneration
{
    Uniform,    // Lights are placed uniform distance from eachother.
    Random,     // Lights are randomly placed within the bounds.
};

class ConfigurationSettings
{
public:
    ConfigurationSettings();

    uint16_t    WindowWidth;
    uint16_t    WindowHeight;
    bool        FullScreen;

    std::string SceneFileName;
    float       SceneScaleFactor;

    glm::vec3   CameraPosition;
    glm::quat   CameraRotation;
    float       CameraPivotDistance;

    // For different scenes, different camera speeds may be required.
    float       NormalCameraSpeed;
    float       FastCameraSpeed;

    // Light
    std::vector<Light> Lights;

    // Light generation properties.
    LightGeneration LightGenerationMethod;
    glm::vec3       LightsMinBounds;
    glm::vec3       LightsMaxBounds;
    float           MinSpotAngle;
    float           MaxSpotAngle;
    float           MinRange;
    float           MaxRange;
    bool            GeneratePointLights;
    bool            GenerateSpotLights;
    bool            GenerateDirectionalLights;

    bool Load( const std::wstring& fileName );
    // Reload configuration settings from previously loaded file.
    bool Reload();
    bool Save( const std::wstring& fileName = L"" );

    // Set to true if the config file version of the serialized file is outdated.
    // If this is true, the configuration file should be upgraded to the latest version (reserialized).
    bool        UpgradeConfigFile;
protected:

private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize( Archive& ar, const unsigned int version );

    // The file that is used to load/save this configuration.
    std::wstring m_Filename;

};

#include "ConfigurationSettings.inl"

BOOST_CLASS_VERSION( ConfigurationSettings, 6 );
