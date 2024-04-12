#include <Serialization.h>

template<class Archive>
void ConfigurationSettings::serialize( Archive& ar, const unsigned int version )
{
    // If the version of the configuration settings being deserialized is not the same
    // as the current version of the class, then we need to update the configuration files 
    // to the new version.
    // Check the status of this value in your application and resave the configuration file
    // using the ConfigurationSettings::Save function.
    UpgradeConfigFile = boost::serialization::version<ConfigurationSettings>::value != version;

    ar & BOOST_SERIALIZATION_NVP( WindowWidth );
    ar & BOOST_SERIALIZATION_NVP( WindowHeight );
    ar & BOOST_SERIALIZATION_NVP( FullScreen );
    ar & BOOST_SERIALIZATION_NVP( SceneFileName );
    ar & BOOST_SERIALIZATION_NVP( SceneScaleFactor );
    if ( version > 0 )
    {
        ar & BOOST_SERIALIZATION_NVP( CameraPosition );
        ar & BOOST_SERIALIZATION_NVP( CameraRotation );
    }
    if ( version > 1 )
    {
        ar & BOOST_SERIALIZATION_NVP( NormalCameraSpeed );
        ar & BOOST_SERIALIZATION_NVP( FastCameraSpeed );
    }
    if ( version > 2 )
    {
        ar & BOOST_SERIALIZATION_NVP( CameraPivotDistance );
    }
    if ( version > 3 )
    {
        ar & BOOST_SERIALIZATION_NVP( Lights );
    }
    if ( version > 4 )
    {
        // Light generation properties.
        ar & BOOST_SERIALIZATION_NVP( LightsMinBounds );
        ar & BOOST_SERIALIZATION_NVP( LightsMaxBounds );
        ar & BOOST_SERIALIZATION_NVP( MinSpotAngle );
        ar & BOOST_SERIALIZATION_NVP( MaxSpotAngle );
        ar & BOOST_SERIALIZATION_NVP( MinRange );
        ar & BOOST_SERIALIZATION_NVP( MaxRange );
        ar & BOOST_SERIALIZATION_NVP( GeneratePointLights );
        ar & BOOST_SERIALIZATION_NVP( GenerateSpotLights );
        ar & BOOST_SERIALIZATION_NVP( GenerateDirectionalLights );
    }
    if ( version > 5 )
    {
        ar & BOOST_SERIALIZATION_NVP( LightGenerationMethod );
    }
}