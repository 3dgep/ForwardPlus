#pragma once

#include <Scene.h>
#include <DependencyTracker.h>

struct aiMaterial;
struct aiMesh;
struct aiNode;

class Material;
class Buffer;
class Mesh;
class Texture;
class Camera;
class SceneNode;

// A model base class.
// Implements a basic model loader using Assimp.
class SceneBase : public Scene
{
public:
    typedef Scene base;

    virtual bool LoadFromFile( const std::wstring& fileName );
    virtual bool LoadFromString( const std::string& scene, const std::string& format );
    virtual void Render( RenderEventArgs& renderArgs );

    virtual std::shared_ptr<SceneNode> GetRootNode() const;

    virtual void Accept( Visitor& visitor );

protected:
    friend class ProgressHandler;

    SceneBase();
    virtual ~SceneBase();

    virtual void OnLoadingProgress( ProgressEventArgs& e );
    virtual void OnFileChanged( FileChangeEventArgs& e );

    virtual std::shared_ptr<Buffer> CreateFloatVertexBuffer( const float* data, unsigned int count, unsigned int stride ) const = 0;
    virtual std::shared_ptr<Buffer> CreateUIntIndexBuffer( const unsigned int* data, unsigned int sizeInBytes ) const = 0;

    virtual std::shared_ptr<Mesh> CreateMesh() const = 0;
    virtual std::shared_ptr<Material> CreateMaterial() const = 0;
    virtual std::shared_ptr<Texture> CreateTexture( const std::wstring& fileName ) const = 0;
    virtual std::shared_ptr<Texture> CreateTexture2D( uint16_t width, uint16_t height ) = 0;

    virtual std::shared_ptr<Texture> GetDefaultTexture() = 0;

private:
    typedef std::map<std::string, std::shared_ptr<Material> > MaterialMap;
    typedef std::vector< std::shared_ptr<Material> > MaterialList;
    typedef std::vector< std::shared_ptr<Mesh> > MeshList;

    MaterialMap m_MaterialMap;
    MaterialList m_Materials;
    MeshList m_Meshes;

    std::shared_ptr<SceneNode> m_pRootNode;

    void ImportMaterial( const aiMaterial& material, fs::path parentPath );
    void ImportMesh( const aiMesh& mesh );
    std::shared_ptr<SceneNode> ImportSceneNode( std::shared_ptr<SceneNode> parent, aiNode* aiNode );

    // Dependency tracker will notify us if we need to reload the scene.
    DependencyTracker m_DependencyTracker;

    Event::ScopedConnections m_Connections;
    std::wstring m_SceneFile;

    // Has the file changed on disk?
    bool m_bFileChanged;
    typedef std::unique_lock<std::recursive_mutex> MutexLock;
    std::recursive_mutex m_Mutex;
};
