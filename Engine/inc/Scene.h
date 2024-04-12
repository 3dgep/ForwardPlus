#pragma once

#include "Object.h"
#include "Events.h"

class SceneNode;
class Camera;
class RenderEventArgs;
class Visitor;

class Scene : public Object
{
public:
    /**
     * Load a scene from a file on disc.
     */
    virtual bool LoadFromFile( const std::wstring& fileName ) = 0;
    /**
     * Load a scene from a string.
     * The scene can be preloaded into a byte array and the 
     * scene can be loaded from the loaded byte array.
     * 
     * @param scene The byte encoded scene file.
     * @param format The format of the scene file. Supported formats are: TODO: get a list of supported formats from ASSIMP.
     */
    virtual bool LoadFromString( const std::string& scene, const std::string& format ) = 0;
    virtual void Render( RenderEventArgs& renderEventArgs ) = 0;

    virtual std::shared_ptr<SceneNode> GetRootNode() const = 0;

    virtual void Accept( Visitor& visitor ) = 0;

    // Register for the progress callback to be notified of scene loading progress.
    ProgressEvent LoadingProgress;

protected:
    virtual void OnLoadingProgress( ProgressEventArgs& e );
};