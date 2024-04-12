#include <GraphicsTestPCH.h>
#include <Application.h>

#include <RenderDevice.h>
#include <RenderWindow.h>
#include <ProgressWindow.h>
#include <PipelineState.h>
#include <Scene.h>
#include <SceneNode.h>
#include <Shader.h>
#include <ShaderParameter.h>
#include <Texture.h>
#include <SamplerState.h>
#include <Material.h>
#include <Light.h>
#include <Mesh.h>
#include <ConstantBuffer.h>
#include <StructuredBuffer.h>
#include <Camera.h>
#include <HighResolutionTimer.h>
#include <Query.h>

#include <ConfigurationSettings.h>

#include <RenderTechnique.h>
#include <ClearRenderTargetPass.h>
#include <CopyBufferPass.h>
#include <CopyTexturePass.h>
#include <GenerateMipMapsPass.h>
#include <OpaquePass.h>
#include <TransparentPass.h>
#include <LightsPass.h>
#include <LightPickingPass.h>
#include <PostprocessPass.h>
#include <DeferredLightingPass.h>
#include <BeginQueryPass.h>
#include <EndQueryPass.h>
#include <DispatchPass.h>
#include <InvokeFunctionPass.h>
#include <Statistic.h>

enum class RenderingTechnique
{
    Forward,
    Deferred,
    ForwardPlus,
    NumTechniques
};

uint32_t g_NumLightsToGenerate = 2;

// Which rendering technique to use for rendering the scene.
RenderingTechnique g_RenderingTechnique = RenderingTechnique::ForwardPlus;

ConfigurationSettings g_Config;

Application g_Application;

// The name of the render device.
std::string g_DeviceName;

std::shared_ptr<Scene> g_pScene = nullptr;

// Cornflower blue
glm::vec4 g_ClearColor( 0.39f, 0.58f, 0.93f, 1.0f );

// Used to track the movement of the mouse during mouse click events.
glm::vec2 g_PreviousMousePosition;

// Some scenes for rendering lights and stuff...
std::shared_ptr<Scene> g_Sphere = nullptr;
std::shared_ptr<Scene> g_Cone = nullptr;
std::shared_ptr<Scene> g_Axis = nullptr;
std::shared_ptr<Scene> g_Plane = nullptr;
std::shared_ptr<Scene> g_Arrow = nullptr;

// Generate a full screen quad that can be used to render post-process effects
// or used to debug textures on screen.
std::shared_ptr<Scene> g_FullScreenQuad = nullptr;

// Shaders that are used in this demo.
std::shared_ptr<Shader> g_pVertexShader;
std::shared_ptr<Shader> g_pPixelShader;
// Pixel shader for rendering the lights as geometry in the scene.
std::shared_ptr<Shader> g_pLightPixelShader;
// Pixel shader for rendering the picking texture.
std::shared_ptr<Shader> g_pLightPickingPixelShader;
// Render materials that should be unlit.
std::shared_ptr<Shader> g_pUnlitPixelShader;
// For debugging textures
std::shared_ptr<Shader> g_pDebugTexturePixelShader;
// For debugging depth textures
std::shared_ptr<Shader> g_pDebugDepthTexturePixelShader;
// For debugging stencil textures
std::shared_ptr<Shader> g_pDebugStencilTexturePixelShader;

// For light culling in compute shader
std::shared_ptr<Shader> g_pLightCullingComputeShader;
// Compute the frustums for light culling.
std::shared_ptr<Shader> g_pComputeFrustumsComputeShader;
// Pixel shader for Forward+
std::shared_ptr<Shader> g_pForwardPlusPixelShader;
// For the light culling compute shader, the number of threads per block (in each dimension)
uint16_t g_LightCullingBlockSize = 16;

// Pixels shader for deferred lighting pass
std::shared_ptr<Shader> g_pDeferredLightingPixelShader;

// Geometry shader for G-Buffer generation
std::shared_ptr<Shader> g_pGeometryPixelShader;

std::shared_ptr<SamplerState> g_LinearClampSampler;
std::shared_ptr<SamplerState> g_LinearRepeatSampler;

// Pipeline state for rendering opaque geometry.
std::shared_ptr<PipelineState> g_pOpaquePipeline;
// Pipeline for rendering transparent geometry.
std::shared_ptr<PipelineState> g_pTransparentPipeline;
// Pipeline state for rendering the lights as geometry in the scene.
std::shared_ptr<PipelineState> g_pLightsPipelineBack;
std::shared_ptr<PipelineState> g_pLightsPipelineFront;
// Pipeline state for generating the picking texture for the lights.
std::shared_ptr<PipelineState> g_pLightPickingPipeline;

// Pipeline for rendering unlit objects.
std::shared_ptr<PipelineState> g_pUnlitPipeline;
// Pipeline for G-buffer pass.
std::shared_ptr<PipelineState> g_pGeometryPipeline;
// Pipeline for debugging G-buffer textures
std::shared_ptr<PipelineState> g_pDebugTexturePipeline;
// Pipeline for debugging textures with blending enabled.
std::shared_ptr<PipelineState> g_pDebugTextureWithBlendingPipeline;
// Pipeline for debugging depth textures
std::shared_ptr<PipelineState> g_pDebugDepthTexturePipeline;

// Pipeline for deferred lighting pass (first pass to determine lit pixels)
std::shared_ptr<PipelineState> g_pDeferredLightingPipeline1;
// Pipeline for deferred lighting pass (second pass to light lit pixels)
std::shared_ptr<PipelineState> g_pDeferredLightingPipeline2;
// Pipeline for deferred directional lights (only require one pass)
std::shared_ptr<PipelineState> g_pDirectionalLightsPipeline;

// Pipeline for depth pre-pass for forward+ rendering technique.
// (Could also be used for regular forward rendering)
std::shared_ptr<PipelineState> g_pDepthPrepassPipeline;

// Pipeline for forward+ rendering.
std::shared_ptr<PipelineState> g_pForwardPlusOpaquePipeline;
std::shared_ptr<PipelineState> g_pForwardPlusTransparentPipeline;

// Staging texture for light picking.
std::shared_ptr<Texture> g_LightPickingTexture;

// Render target for GBuffer
std::shared_ptr<RenderTarget> g_pGBufferRenderTarget;
// A render target that has only a depth target (useful if only the depth/stencil buffer needs to be updated)
std::shared_ptr<RenderTarget> g_pDepthOnlyRenderTarget;
// A render target that has only a color target (useful if you don't need to perform depth/stencil testing)
std::shared_ptr<RenderTarget> g_pColorOnlyRenderTarget;
// A render target for generating the light picking texture
std::shared_ptr<RenderTarget> g_pLightPickingRenderTarget;

// A render technique for forward rendering.
RenderTechnique g_ForwardTechnique;
// A render technique for deferred rendering.
RenderTechnique g_DeferredTechnique;
// A render technique for forward plus rendering.
RenderTechnique g_ForwardPlusTechnique;

// Constant buffer to store the number of groups executed in a dispatch.
__declspec( align( 16 ) ) struct DispatchParams
{
    glm::uvec3 m_NumThreadGroups;
    uint32_t m_Padding0;        // Pad to 16 bytes.
    glm::uvec3 m_NumThreads;
    uint32_t m_Padding1;        // Pad to 16 bytes.
};
std::shared_ptr<ConstantBuffer> g_pDispatchParamsConstantBuffer;

__declspec( align( 16 ) ) struct ScreenToViewParams
{
    glm::mat4x4 m_InverseProjectionMatrix;
    glm::vec2 m_ScreenDimensions;
};
std::shared_ptr<ConstantBuffer> g_pScreenToViewParamsConstantBuffer;

__declspec( align( 16 ) ) struct Frustum
{
    glm::vec4 planes[4];    // 64 Bytes
};
// Grid frustums for light culling.
std::shared_ptr<StructuredBuffer> g_pGridFrustums;
// The light index list stores the light indices per tile.
// The light index list is produced by the light culling compute shader
// and consumed by the forward+ pixel shader.
std::shared_ptr<StructuredBuffer> g_pLightIndexListOpaque;
std::shared_ptr<StructuredBuffer> g_pLightIndexListTransparent;

// Keep track of the current index in the light list.
std::shared_ptr<StructuredBuffer> g_pLightListIndexCounterOpaque;
std::shared_ptr<StructuredBuffer> g_pLightListIndexCounterTransparent;
// For the light index list, we need to make a guess as to the average 
// number of overlapping lights per tile. It could be possible to refine this
// value at runtime (if it is underestimated) but for now, I'll just take a guess
// of about 200 lights (which may be an overestimation, but better over than under). 
// The total size of the buffer will be determined by the grid size but for 16x16
// tiles at 1080p, we would need 120x68 tiles * 200 light indices * 4 bytes (to store a uint)
// making the light index list 6,528,000 bytes (6.528 MB)
const uint32_t AVERAGE_OVERLAPPING_LIGHTS_PER_TILE = 200u;

// The light grid stores the starting index in the light index list and the 
// light count per tile. The light grid can be much more conservative than the light
// index list because we only need to store a single uint per tile (the start 
// offset and the number of lights in a tile can be packed in a single uint).
// The light grid will be produced by the light culling compute shader
// and consumed by the forward+ pixel shader.
std::shared_ptr<Texture> g_pLightGridOpaque;
std::shared_ptr<Texture> g_pLightGridTransparent;


// For debugging of the light culling shader.
std::shared_ptr<Texture> g_pLightCullingDebugTexture;
// Heatmap texture for light culling debug.
std::shared_ptr<Texture> g_pLightCullingHeatMap;

// A render technique for generating the picking texture.
RenderTechnique g_LightPickingTechnique;

// Timer query for entire frame.
std::shared_ptr<Query> g_pFrameQuery;
Statistic g_FrameStatistic;

// Timer query for forward rendering opaque pass.
std::shared_ptr<Query> g_pForwardOpaqueQuery;
Statistic g_ForwardOpaqueStatistic;

// Timer query for forward rendering transparent pass.
std::shared_ptr<Query> g_pForwardTransparentQuery;
Statistic g_ForwardTransparentStatistic;

// Timer query for deferred lighting geometry pass.
std::shared_ptr<Query> g_pDeferredGeometryQuery;
Statistic g_DeferredGeometryStatistic;

// Timer query for deferred lighting pass.
std::shared_ptr<Query> g_pDeferredLightingQuery;
Statistic g_DeferredLightingStatistic;

// Timer query for deferred transparent pass.
std::shared_ptr<Query> g_pDeferredTransparentQuery;
Statistic g_DeferredTransparentStatistic;

// Timer query for Forward+ depth prepass.
std::shared_ptr<Query> g_pForwardPlusDepthPrepassQuery;
Statistic g_ForwardPlusDepthPrepassStatistic;

// Timer query for Forward+ light culling pass.
std::shared_ptr<Query> g_pForwardPlusLightCullingQuery;
Statistic g_ForwardPlusLightCullingStatistic;

// Timer query for Forward+ opaque pass.
std::shared_ptr<Query> g_pForwardPlusOpaqueQuery;
Statistic g_ForwardPlusOpaqueStatistic;

// Timer query for Forward+ transparent pass.
std::shared_ptr<Query> g_pForwardPlusTransparentQuery;
Statistic g_ForwardPlusTransparentStatistic;

double g_FrameTime = 0.0;

double g_RunningTime = 0.0;
uint32_t g_FrameCount = 0;

Camera g_Camera;

struct CameraMovement
{
    // Translation movement
    float Forward, Back, Left, Right, Up, Down;
    // Rotation movement
    float RollCW, RollCCW;
    float Pitch, Yaw;
    // Move in/out from pivot point.
    float PivotTranslate;
    // Do you want to go faster?
    bool TranslateFaster;
    bool RotateFaster;

    CameraMovement()
        : Forward( 0.0f )
        , Back( 0.0f )
        , Left( 0.0f )
        , Right( 0.0f )
        , Up( 0.0f )
        , Down( 0.0f )
        , RollCW( 0.0f )
        , RollCCW( 0.0f )
        , Pitch( 0.0f )
        , Yaw( 0.0f )
        , PivotTranslate( 0.0f )
        , TranslateFaster( false )
        , RotateFaster( false )
    {}
};
CameraMovement g_CameraMovement;

// Pointer to the currently selected light.
Light* g_pCurrentLight = nullptr;
// The index of the currently selected light in the
// lights array.
uint32_t g_uiCurrentLightIndex = 0;
// If true, the position of the currently 
// selected light will track the pivot point of the camera.
bool g_bLightTracksCamera = false;

// Structured buffer to store lighting information.
std::shared_ptr<StructuredBuffer> g_pLightsStructuredBuffer = nullptr;

// Screen dimensions are read from the config file.
unsigned int g_WindowWidth;
unsigned int g_WindowHeight;

// Toggle animations (lights moving).
bool g_Animate = false;
// For forward rendering, whether to render the lights in the scene as geometry or not.
bool g_RenderLights = false;

// Set to true when the render targets and textures need to be resized (because the application window was resized)
bool g_bResizePending = false;

// Passes
// The pass for rendering the lights in the scene.
std::shared_ptr<LightsPass> g_LightsPassFront;
std::shared_ptr<LightsPass> g_LightsPassBack;
// The pass used to render the camera's pivot point as a 6 point axis
std::shared_ptr<OpaquePass> g_PivotPointPass;
// Pass for rendering transparent geometry.
std::shared_ptr<TransparentPass> g_TransparentPass;
// Passes for debugging various textures of the g-buffer pass
std::shared_ptr<PostprocessPass> g_DebugTexture0Pass;
std::shared_ptr<PostprocessPass> g_DebugTexture1Pass;
std::shared_ptr<PostprocessPass> g_DebugTexture2Pass;
std::shared_ptr<PostprocessPass> g_DebugTexture3Pass;
// Debug pass for Forward+ pixel shader.
std::shared_ptr<PostprocessPass> g_ForwardPlusDebugPass;

// Forward+ light culling pass.
std::shared_ptr<DispatchPass> g_LightCullingDispatchPass;

// Ant Tweak bars
TwBar* g_pRenderingTechniqueTweakBar = nullptr;
TwBar* g_pLightsTweakBar = nullptr;
TwBar* g_pGenerateLightsTweakBar = nullptr;

// Create the Ant Tweak bars.
void CreateAntTweakBar();

// Update the lights in the scene.
// Compute the lights view space position and direction.
void UpdateLights();

// Generate lights using the specified methods.
// Other properties are specified in the configuration settings.
void GenerateLights( LightGeneration genMethod, uint32_t numLights );

// If the number of lights in the scene changes, we need to recompile
// the pixel shader and update the constant buffer for the lights.
void UpdateNumLights();

// Specify the tile size for tiled deferred rendering.
// Valid values are 8, 16, and 32.
void SetThreadGroupBlockSize( uint16_t blockSize );

// Update the light culling frustums used by 
// the tiled renderers.
void UpdateGridFrustrums();

void OnUpdate( UpdateEventArgs& e );

void OnPreRender( RenderEventArgs& e );
void OnRender( RenderEventArgs& e );
void OnPostRender( RenderEventArgs& e );

// Keyboard event callbacks
void OnKeyPressed( KeyEventArgs& e );
void OnKeyReleased( KeyEventArgs& e );

// Mouse event callbacks
void OnMouseButtonPressed( MouseButtonEventArgs& e );
void OnMouseButtonReleased( MouseButtonEventArgs& e );
void OnMouseMoved( MouseMotionEventArgs& e );
void OnMouseWheel( MouseWheelEventArgs& e );

// Joystick event callbacks
void OnJoystickButtonPressed( JoystickButtonEventArgs& e );
void OnJoystickButtonReleased( JoystickButtonEventArgs& e );
void OnJoystickPOV( JoystickPOVEventArgs& e );
void OnJoystickAxis( JoystickAxisEventArgs& e );

// Window event callbacks.
void OnWindowResize( ResizeEventArgs& e );
void OnWindowClose( WindowCloseEventArgs& e );

// File modification callback.
void OnFileChanged( FileChangeEventArgs& e );

// Set the index of the currently selected light.
void SetCurrentLight( uint32_t newIndex );

// Resize render targets and textures. Should not be called too often,
// so resizing is delayed until the beginning of the render function.
void ResizeBuffers( unsigned int width, unsigned int height );

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow )
{
    // Make sure our current directory is set to the running application's working directory.
    #define MAX_FILE_PATH 256
    WCHAR moduleFilename[MAX_FILE_PATH];
    DWORD len = GetModuleFileNameW( 0, moduleFilename, MAX_FILE_PATH );
    if ( len > 0 && len < MAX_FILE_PATH )
    {
        fs::path modulePath( moduleFilename );
        fs::current_path( modulePath.parent_path() );
    }

    // Register directory change listener for the assets folder.
    g_Application.RegisterDirectoryChangeListener( L"../Assets" );

    // Load configuration settings.
    int numArgs;
    LPWSTR* commandLineArguments = CommandLineToArgvW( GetCommandLineW(), &numArgs );

    std::wstring configFileName = L"../Conf/DefaultConfiguration.3dgep";
    // Parse command line arguments.
    for ( int i = 0; i < numArgs; i++ )
    {
        if ( wcscmp( commandLineArguments[i], L"-c" ) == 0 || wcscmp( commandLineArguments[i], L"--config" ) == 0 )
        {
            configFileName = commandLineArguments[++i];
        }
    }

    if ( !g_Config.Load( configFileName ) )
    {
        ReportError( "Failed to load configuration file " + ConvertString( configFileName ) );
        return -1;
    }

    g_NumLightsToGenerate = static_cast<uint32_t>( g_Config.Lights.size() );
    g_WindowWidth = g_Config.WindowWidth;
    g_WindowHeight = g_Config.WindowHeight;

    std::string windowName = "Graphics Test ";
#if defined(_WIN32_WINNT_WIN10) && _WIN32_WINNT == _WIN32_WINNT_WIN10
    windowName += "/ Win 10 ";
#elif defined(_WIN32_WINNT_WINBLUE) && _WIN32_WINNT == _WIN32_WINNT_WINBLUE
    windowName += "/ Win 8.1 ";
#elif defined(_WIN32_WINNT_WIN8) && _WIN32_WINNT == _WIN32_WINNT_WIN8
    windowName += "/ Win 8 ";
#elif defined(_WIN32_WINNT_WIN7) && _WIN32_WINNT == _WIN32_WINNT_WIN7
    windowName += "/ Win 7 ";
#endif

#if defined(_WIN64)
    windowName += "/ x64 ";
#elif defined(_WIN32)
    windowName += "/ x86 ";
#endif

#if defined(_DEBUG)
    windowName += "/ Debug ";
#endif

    windowName += "[ " + g_Config.SceneFileName + " ]";

    RenderWindow& renderWindow = g_Application.CreateRenderWindow(windowName, g_Config.WindowWidth, g_Config.WindowHeight);

    // Create and show a loading window.
    ProgressWindow& loadingWindow = g_Application.CreateProgressWindow( "Loading...", 400, 50 );
    // Set this to the total number of scenes that are to be loaded.
    loadingWindow.SetTotalProgress( 6.0f );
    loadingWindow.ShowWindow();

    RenderDevice& renderDevice = g_Application.GetRenderDevice();
    // Bind the update progress event so we can update the progress bar of the progress window.
    renderDevice.LoadingProgress += boost::bind( &ProgressWindow::UpdateProgress, &loadingWindow, _1);

    g_DeviceName = renderDevice.GetDeviceName();

    // Create query objects for querying GPU timestamps.
    // GPU timer query for entire frame.
    g_pFrameQuery = renderDevice.CreateQuery( Query::QueryType::Timer, 2 );
    // GPU timer query for rendering opaque geometry using forward rendering.
    g_pForwardOpaqueQuery = renderDevice.CreateQuery( Query::QueryType::Timer, 2 );
    // GPU timer query for rendering transparent geometry using forward rendering.
    g_pForwardTransparentQuery = renderDevice.CreateQuery( Query::QueryType::Timer, 2 );
    // GPU timer query for rendering G-buffer for deferred rendering.
    g_pDeferredGeometryQuery = renderDevice.CreateQuery( Query::QueryType::Timer, 2 );
    // GPU timer query for rendering lighting in deferred rendering.
    g_pDeferredLightingQuery = renderDevice.CreateQuery( Query::QueryType::Timer, 2 );
    // GPU timer query for rendering transparent geometry in deferred rendering.
    g_pDeferredTransparentQuery = renderDevice.CreateQuery( Query::QueryType::Timer, 2 );
    // GPU Timer query for forward+ depth prepass
    g_pForwardPlusDepthPrepassQuery = renderDevice.CreateQuery( Query::QueryType::Timer, 2 );
    // GPU timer query for forward+ light culling.
    g_pForwardPlusLightCullingQuery = renderDevice.CreateQuery( Query::QueryType::Timer, 2 );
    // GPU timer query for forward+ opaque pass.
    g_pForwardPlusOpaqueQuery = renderDevice.CreateQuery( Query::QueryType::Timer, 2 );
    // GPU timer query for forward+ transparent pass.
    g_pForwardPlusTransparentQuery = renderDevice.CreateQuery( Query::QueryType::Timer, 2 );

    // TEST Cubemap loading.
//    std::shared_ptr<Texture> CubeTexture = renderDevice.CreateTextureCube( L"../Assets/textures/mountain_cube.dds" );

    g_Camera.SetTranslate( g_Config.CameraPosition );
    g_Camera.SetRotate( g_Config.CameraRotation );
    g_Camera.SetPivotDistance( g_Config.CameraPivotDistance );

    g_Camera.SetViewport( Viewport( 0, 0, g_Config.WindowWidth, g_Config.WindowHeight ) );
    g_Camera.SetProjectionRH( 45.0f, g_Config.WindowWidth / (float)g_Config.WindowHeight, 0.1f, 1000.0f );
    
    // Load a scene
    g_pScene = renderDevice.CreateScene();

    fs::path configFilePath( configFileName );
    fs::path sceneFilePath( g_Config.SceneFileName );


    // Scene file is described relative to the configuration file.
    if ( !g_pScene->LoadFromFile( ( configFilePath.parent_path() / sceneFilePath ).wstring() ) )
    {
        ReportError( "Unable to load scene file from " + sceneFilePath.string() );
    }

    // Scale the scene to fit the view.
    g_pScene->GetRootNode()->SetLocalTransform( glm::scale( glm::vec3( g_Config.SceneScaleFactor ) ) );

    // Load some shaders
    g_pVertexShader = renderDevice.CreateShader();
    g_pPixelShader = renderDevice.CreateShader();
    g_pLightPixelShader = renderDevice.CreateShader();
    g_pLightPickingPixelShader = renderDevice.CreateShader();
    g_pUnlitPixelShader = renderDevice.CreateShader();
    g_pGeometryPixelShader = renderDevice.CreateShader();
    g_pDebugTexturePixelShader = renderDevice.CreateShader();
    g_pDebugDepthTexturePixelShader = renderDevice.CreateShader();
    g_pDebugStencilTexturePixelShader = renderDevice.CreateShader();
    g_pDeferredLightingPixelShader = renderDevice.CreateShader();
    g_pLightCullingComputeShader = renderDevice.CreateShader();
    g_pComputeFrustumsComputeShader = renderDevice.CreateShader();
    g_pForwardPlusPixelShader = renderDevice.CreateShader();
    
    g_pVertexShader->LoadShaderFromFile( Shader::VertexShader, L"../Assets/shaders/ForwardRendering.hlsl", Shader::ShaderMacros(), "VS_main", "latest" );
    g_pPixelShader->LoadShaderFromFile( Shader::PixelShader, L"../Assets/shaders/ForwardRendering.hlsl", Shader::ShaderMacros(), "PS_main", "latest" );
    g_pLightPixelShader->LoadShaderFromFile( Shader::PixelShader, L"../Assets/shaders/ForwardRendering.hlsl", Shader::ShaderMacros(), "PS_light", "latest" );
    g_pLightPickingPixelShader->LoadShaderFromFile( Shader::PixelShader, L"../Assets/shaders/DeferredRendering.hlsl", Shader::ShaderMacros(), "PS_LightPicking", "latest" );
    g_pUnlitPixelShader->LoadShaderFromFile( Shader::PixelShader, L"../Assets/shaders/ForwardRendering.hlsl", Shader::ShaderMacros(), "PS_unlit", "latest" );
    g_pGeometryPixelShader->LoadShaderFromFile( Shader::PixelShader, L"../Assets/shaders/DeferredRendering.hlsl", Shader::ShaderMacros(), "PS_Geometry", "latest" );
    g_pDebugTexturePixelShader->LoadShaderFromFile( Shader::PixelShader, L"../Assets/shaders/DeferredRendering.hlsl", Shader::ShaderMacros(), "PS_DebugTexture", "latest" );
    g_pDebugDepthTexturePixelShader->LoadShaderFromFile( Shader::PixelShader, L"../Assets/shaders/DeferredRendering.hlsl", Shader::ShaderMacros(), "PS_DebugDepthTexture", "latest" );
    g_pDebugStencilTexturePixelShader->LoadShaderFromFile( Shader::PixelShader, L"../Assets/shaders/DeferredRendering.hlsl", Shader::ShaderMacros(), "PS_DebugStencilTexture", "latest" );
    g_pDeferredLightingPixelShader->LoadShaderFromFile( Shader::PixelShader, L"../Assets/shaders/DeferredRendering.hlsl", Shader::ShaderMacros(), "PS_DeferredLighting", "latest" );
    g_pLightCullingComputeShader->LoadShaderFromFile( Shader::ComputeShader, L"../Assets/shaders/ForwardPlusRendering.hlsl", Shader::ShaderMacros(), "CS_main", "cs_5_0" );
    g_pComputeFrustumsComputeShader->LoadShaderFromFile( Shader::ComputeShader, L"../Assets/shaders/ForwardPlusRendering.hlsl", Shader::ShaderMacros(), "CS_ComputeFrustums", "cs_5_0" );
    g_pForwardPlusPixelShader->LoadShaderFromFile( Shader::PixelShader, L"../Assets/shaders/ForwardPlusRendering.hlsl", Shader::ShaderMacros(), "PS_main", "latest" );

    // Create a staging texture for light picking.
    Texture::TextureFormat lightPickingTextureFormat(
        Texture::Components::R,
        Texture::Type::UnsignedInteger,
        1,
        16, 0, 0, 0, 0, 0 );
    g_LightPickingTexture = renderDevice.CreateTexture2D( g_Config.WindowWidth, g_Config.WindowHeight, 1, lightPickingTextureFormat, CPUAccess::Read );
    // Create a texture for light picking.
    std::shared_ptr<Texture> lightPickingTexure = renderDevice.CreateTexture2D( g_Config.WindowWidth, g_Config.WindowHeight, 1, lightPickingTextureFormat );

    // Depth/stencil buffer for light picking
    Texture::TextureFormat lightPickingDepthStencilTextureFormat(
        Texture::Components::DepthStencil,
        Texture::Type::UnsignedNormalized,
        1,
        0, 0, 0, 0, 24, 8 );
    std::shared_ptr<Texture> lightPickingDepthStencilTexture = renderDevice.CreateTexture2D( g_Config.WindowWidth, g_Config.WindowHeight, 1, lightPickingDepthStencilTextureFormat );

    // Create a render target for the light picking texture
    g_pLightPickingRenderTarget = renderDevice.CreateRenderTarget();
    g_pLightPickingRenderTarget->AttachTexture( RenderTarget::AttachmentPoint::Color0, lightPickingTexure );
    g_pLightPickingRenderTarget->AttachTexture( RenderTarget::AttachmentPoint::DepthStencil, lightPickingDepthStencilTexture );

    // Number of samples for multi sample textures.
    uint8_t numSamples = 1;

    // Setup the textures for deferred rendering.
    
    // Depth/stencil buffer
    Texture::TextureFormat depthStencilTextureFormat(
        Texture::Components::DepthStencil,
        Texture::Type::UnsignedNormalized,
        numSamples,
        0, 0, 0, 0, 24, 8 );
    std::shared_ptr<Texture> depthStencilTexture = renderDevice.CreateTexture2D( g_Config.WindowWidth, g_Config.WindowHeight, 1, depthStencilTextureFormat );

    // Diffuse albedo buffer (Color1) 
    Texture::TextureFormat diffuseTextureFormat(
        Texture::Components::RGBA,
        Texture::Type::UnsignedNormalized,
        numSamples,
        8, 8, 8, 8, 0, 0 );
    std::shared_ptr<Texture> diffuseTexture = renderDevice.CreateTexture2D( g_Config.WindowWidth, g_Config.WindowHeight, 1, diffuseTextureFormat );

    // Specular buffer (Color2)
    Texture::TextureFormat specularTextureFormat(
        Texture::Components::RGBA,
        Texture::Type::UnsignedNormalized,
        numSamples,
        8, 8, 8, 8, 0, 0 );
    std::shared_ptr<Texture> specularTexture = renderDevice.CreateTexture2D( g_Config.WindowWidth, g_Config.WindowHeight, 1, specularTextureFormat );

    // Normal buffer (Color3)
    Texture::TextureFormat normalTextureFormat(
        Texture::Components::RGBA,
        Texture::Type::Float,
        numSamples,
        32, 32, 32, 32, 0, 0 );
    std::shared_ptr<Texture> normalTexture = renderDevice.CreateTexture2D( g_Config.WindowWidth, g_Config.WindowHeight, 1, normalTextureFormat );

    // Create a render target for the geometry pass.
    g_pGBufferRenderTarget = renderDevice.CreateRenderTarget();
    // Use the render window's color attachment point for the "light accumulation" texture (no reason to have an additional buffer for this, that I'm aware of..)
    g_pGBufferRenderTarget->AttachTexture( RenderTarget::AttachmentPoint::Color0, renderWindow.GetRenderTarget()->GetTexture(RenderTarget::AttachmentPoint::Color0) );
    g_pGBufferRenderTarget->AttachTexture( RenderTarget::AttachmentPoint::Color1, diffuseTexture );
    g_pGBufferRenderTarget->AttachTexture( RenderTarget::AttachmentPoint::Color2, specularTexture );
    g_pGBufferRenderTarget->AttachTexture( RenderTarget::AttachmentPoint::Color3, normalTexture );
    g_pGBufferRenderTarget->AttachTexture( RenderTarget::AttachmentPoint::DepthStencil, depthStencilTexture );

    // Create a render target with only a depth buffer.
    // This is used for the first (sub) pass of the lighting pass for deferred rendering
    // and for the depth pre-pass of the forward+ technique.
    g_pDepthOnlyRenderTarget = renderDevice.CreateRenderTarget();
    g_pDepthOnlyRenderTarget->AttachTexture( RenderTarget::AttachmentPoint::DepthStencil, renderWindow.GetRenderTarget()->GetTexture( RenderTarget::AttachmentPoint::DepthStencil ) );

    // Create a render target with only a color buffer.
    // Used when depth/stencil operations can be ignored.
    g_pColorOnlyRenderTarget = renderDevice.CreateRenderTarget();
    g_pColorOnlyRenderTarget->AttachTexture( RenderTarget::AttachmentPoint::Color0, renderWindow.GetRenderTarget()->GetTexture( RenderTarget::AttachmentPoint::Color0 ) );

    // Setup rendering pipelines
    // Pipeline for rendering opaque geometry.
    g_pOpaquePipeline = renderDevice.CreatePipelineState();
    g_pOpaquePipeline->SetShader( Shader::VertexShader, g_pVertexShader );
    g_pOpaquePipeline->SetShader( Shader::PixelShader, g_pPixelShader );
    g_pOpaquePipeline->SetRenderTarget( renderWindow.GetRenderTarget() );

    // Blend mode for alpha blending.
    BlendState::BlendMode alphaBlending( true, false,                          // BlendEnabled, LogicOpEnabled
                                         BlendState::BlendFactor::SrcAlpha,         // SrcFactor
                                         BlendState::BlendFactor::OneMinusSrcAlpha );  // DstFactor
    // Blend mode for additive blending.
    BlendState::BlendMode additiveBlending( true, false,                     // BlendEnabled, LogicOpEnabled
                                            BlendState::BlendFactor::One,    // SrcFactor
                                            BlendState::BlendFactor::One );   // DstFactor

    // Depth mode for disabling depth writes (for transparent object for example).
    DepthStencilState::DepthMode disableDepthWrites( true, DepthStencilState::DepthWrite::Disable );
    // Disable depth testing when debugging textures on the screen.
    DepthStencilState::DepthMode disableDepthTesting( false ); 

    // Pipeline for rendering transparent geometry.
    g_pTransparentPipeline = renderDevice.CreatePipelineState();
    g_pTransparentPipeline->SetShader( Shader::VertexShader, g_pVertexShader );
    g_pTransparentPipeline->SetShader( Shader::PixelShader, g_pPixelShader );
    g_pTransparentPipeline->GetBlendState().SetBlendMode( alphaBlending );
    g_pTransparentPipeline->GetDepthStencilState().SetDepthMode( disableDepthWrites );
    g_pTransparentPipeline->GetRasterizerState().SetCullMode( RasterizerState::CullMode::None );
    g_pTransparentPipeline->SetRenderTarget( renderWindow.GetRenderTarget() );

    // Pipeline for rendering back faces of light geometry.
    g_pLightsPipelineBack = renderDevice.CreatePipelineState();
    g_pLightsPipelineBack->SetShader( Shader::VertexShader, g_pVertexShader );
    g_pLightsPipelineBack->SetShader( Shader::PixelShader, g_pLightPixelShader );
    g_pLightsPipelineBack->SetRenderTarget( renderWindow.GetRenderTarget() );
    g_pLightsPipelineBack->GetRasterizerState().SetCullMode( RasterizerState::CullMode::Front );
    g_pLightsPipelineBack->GetDepthStencilState().SetDepthMode( disableDepthWrites );
    g_pLightsPipelineBack->GetBlendState().SetBlendMode( alphaBlending );

    // Pipeline for rendering front faces of light geometry.
    g_pLightsPipelineFront = renderDevice.CreatePipelineState();
    g_pLightsPipelineFront->SetShader( Shader::VertexShader, g_pVertexShader );
    g_pLightsPipelineFront->SetShader( Shader::PixelShader, g_pLightPixelShader );
    g_pLightsPipelineFront->SetRenderTarget( renderWindow.GetRenderTarget() );
    g_pLightsPipelineFront->GetRasterizerState().SetCullMode( RasterizerState::CullMode::Back );
    g_pLightsPipelineFront->GetDepthStencilState().SetDepthMode( disableDepthWrites );
    g_pLightsPipelineFront->GetBlendState().SetBlendMode( alphaBlending );

    // Pipeline for rendering picking texture
    g_pLightPickingPipeline = renderDevice.CreatePipelineState();
    g_pLightPickingPipeline->SetShader( Shader::VertexShader, g_pVertexShader );
    g_pLightPickingPipeline->SetShader( Shader::PixelShader, g_pLightPickingPixelShader );
    g_pLightPickingPipeline->SetRenderTarget( g_pLightPickingRenderTarget );
    g_pLightPickingPipeline->GetRasterizerState().SetCullMode( RasterizerState::CullMode::None );

    // Pipeline for rendering unlit geometry.
    g_pUnlitPipeline = renderDevice.CreatePipelineState();
    g_pUnlitPipeline->SetShader( Shader::VertexShader, g_pVertexShader );
    g_pUnlitPipeline->SetShader( Shader::PixelShader, g_pUnlitPixelShader );
    g_pUnlitPipeline->SetRenderTarget( renderWindow.GetRenderTarget() );

    // Pipeline for G-buffer pass.
    g_pGeometryPipeline = renderDevice.CreatePipelineState();
    g_pGeometryPipeline->SetShader( Shader::VertexShader, g_pVertexShader );
    g_pGeometryPipeline->SetShader( Shader::PixelShader, g_pGeometryPixelShader );
    g_pGeometryPipeline->SetRenderTarget( g_pGBufferRenderTarget );

    // Pipeline for debugging textures on screen.
    g_pDebugTexturePipeline = renderDevice.CreatePipelineState();
    g_pDebugTexturePipeline->SetShader( Shader::VertexShader, g_pVertexShader );
    g_pDebugTexturePipeline->SetShader( Shader::PixelShader, g_pDebugTexturePixelShader );
    g_pDebugTexturePipeline->GetDepthStencilState().SetDepthMode( disableDepthTesting );
    g_pDebugTexturePipeline->SetRenderTarget( g_pColorOnlyRenderTarget );

    // Pipeline for debugging textures on screen (with blending enabled).
    g_pDebugTextureWithBlendingPipeline = renderDevice.CreatePipelineState();
    g_pDebugTextureWithBlendingPipeline->SetShader( Shader::VertexShader, g_pVertexShader );
    g_pDebugTextureWithBlendingPipeline->SetShader( Shader::PixelShader, g_pDebugTexturePixelShader );
    g_pDebugTextureWithBlendingPipeline->GetDepthStencilState().SetDepthMode( disableDepthTesting );
    g_pDebugTextureWithBlendingPipeline->GetBlendState().SetBlendMode( alphaBlending );
    g_pDebugTextureWithBlendingPipeline->SetRenderTarget( g_pColorOnlyRenderTarget );

    // Pipeline for debugging depth textures on screen.
    g_pDebugDepthTexturePipeline = renderDevice.CreatePipelineState();
    g_pDebugDepthTexturePipeline->SetShader( Shader::VertexShader, g_pVertexShader );
    g_pDebugDepthTexturePipeline->SetShader( Shader::PixelShader, g_pDebugDepthTexturePixelShader );
    g_pDebugDepthTexturePipeline->GetDepthStencilState().SetDepthMode( disableDepthTesting );
    g_pDebugDepthTexturePipeline->SetRenderTarget( g_pColorOnlyRenderTarget );

    // Pipeline for deferred lighting (stage 1 to determine lit pixels)
    {
        g_pDeferredLightingPipeline1 = renderDevice.CreatePipelineState();
        g_pDeferredLightingPipeline1->SetShader( Shader::VertexShader, g_pVertexShader );
        g_pDeferredLightingPipeline1->SetRenderTarget( g_pDepthOnlyRenderTarget );

        // Setup rasterizer state
        g_pDeferredLightingPipeline1->GetRasterizerState().SetCullMode( RasterizerState::CullMode::Back );
        g_pDeferredLightingPipeline1->GetRasterizerState().SetDepthClipEnabled( true );

        // Setup depth mode
        // Disable writing to the depth buffer.
        DepthStencilState::DepthMode depthMode( true, DepthStencilState::DepthWrite::Disable ); // Disable depth writes.
        // Pass depth test if the light volume is behind scene geometry.
        depthMode.DepthFunction = DepthStencilState::CompareFunction::Greater;
        g_pDeferredLightingPipeline1->GetDepthStencilState().SetDepthMode( depthMode );

        // Setup stencil mode
        DepthStencilState::StencilMode stencilMode( true ); // Enable stencil operations
        DepthStencilState::FaceOperation faceOperation;
        faceOperation.StencilDepthPass = DepthStencilState::StencilOperation::DecrementClamp;
        stencilMode.StencilReference = 1;
        stencilMode.FrontFace = faceOperation;
        
        g_pDeferredLightingPipeline1->GetDepthStencilState().SetStencilMode( stencilMode );
    }

    // Pipeline for deferred lighting (stage 2 to render lit pixels)
    {
        g_pDeferredLightingPipeline2 = renderDevice.CreatePipelineState();
        g_pDeferredLightingPipeline2->SetShader( Shader::VertexShader, g_pVertexShader );
        g_pDeferredLightingPipeline2->SetShader( Shader::PixelShader, g_pDeferredLightingPixelShader );
        g_pDeferredLightingPipeline2->SetRenderTarget( renderWindow.GetRenderTarget() );

        // Setup rasterizer state.
        g_pDeferredLightingPipeline2->GetRasterizerState().SetCullMode( RasterizerState::CullMode::Front );
        g_pDeferredLightingPipeline2->GetRasterizerState().SetDepthClipEnabled( false );

        // Perform additive blending if a pixel passes the depth/stencil tests.
        g_pDeferredLightingPipeline2->GetBlendState().SetBlendMode( additiveBlending );

        // Setup depth mode
        // Disable depth writes
        DepthStencilState::DepthMode depthMode( true, DepthStencilState::DepthWrite::Disable ); // Disable depth writes.
        depthMode.DepthFunction = DepthStencilState::CompareFunction::GreaterOrEqual;
        g_pDeferredLightingPipeline2->GetDepthStencilState().SetDepthMode( depthMode );

        // Setup stencil mode
        DepthStencilState::StencilMode stencilMode( true );
        DepthStencilState::FaceOperation faceOperation;
        // Render pixel if the depth function passes and the stencil was not un-marked in the previous pass.
        faceOperation.StencilFunction = DepthStencilState::CompareFunction::Equal;
        stencilMode.StencilReference = 1;
        stencilMode.BackFace = faceOperation;
        g_pDeferredLightingPipeline2->GetDepthStencilState().SetStencilMode( stencilMode );

    }
    // Pipeline for directional lights in deferred shader (only requires a single pass)
    {
        g_pDirectionalLightsPipeline = renderDevice.CreatePipelineState();
        g_pDirectionalLightsPipeline->SetShader( Shader::VertexShader, g_pVertexShader );
        g_pDirectionalLightsPipeline->SetShader( Shader::PixelShader, g_pDeferredLightingPixelShader );
        g_pDirectionalLightsPipeline->SetRenderTarget( renderWindow.GetRenderTarget() );
        g_pDirectionalLightsPipeline->GetBlendState().SetBlendMode( additiveBlending );

        // Setup depth mode
        DepthStencilState::DepthMode depthMode( true, DepthStencilState::DepthWrite::Disable ); // Disable depth writes.
        // The full-screen quad that will be used to light pixels will be placed at the far clipping plane.
        // Only light pixels that are "in front" of the full screen quad (exclude sky box pixels)
        depthMode.DepthFunction = DepthStencilState::CompareFunction::Greater;
        g_pDirectionalLightsPipeline->GetDepthStencilState().SetDepthMode( depthMode );
    }

    // Pipeline for depth pre-pass for forward+ rendering technique.
    g_pDepthPrepassPipeline = renderDevice.CreatePipelineState();
    g_pDepthPrepassPipeline->SetShader( Shader::VertexShader, g_pVertexShader );
    // no fragment shader necessary.
    g_pDepthPrepassPipeline->SetRenderTarget( g_pDepthOnlyRenderTarget );

    // Pipeline for Forward+.
    {
        // Opaque pipeline
        g_pForwardPlusOpaquePipeline = renderDevice.CreatePipelineState();
        g_pForwardPlusOpaquePipeline->SetShader( Shader::VertexShader, g_pVertexShader );
        g_pForwardPlusOpaquePipeline->SetShader( Shader::PixelShader, g_pForwardPlusPixelShader );
        g_pForwardPlusOpaquePipeline->SetRenderTarget( renderWindow.GetRenderTarget() );
        DepthStencilState::DepthMode depthMode;
        depthMode.DepthFunction = DepthStencilState::CompareFunction::LessOrEqual; // We need to set depth mode to <= because we have a depth prepass in Forward+
        g_pForwardPlusOpaquePipeline->GetDepthStencilState().SetDepthMode( depthMode );
    }
    {
        // Transparent pipeline.
        g_pForwardPlusTransparentPipeline = renderDevice.CreatePipelineState();
        g_pForwardPlusTransparentPipeline->SetShader( Shader::VertexShader, g_pVertexShader );
        g_pForwardPlusTransparentPipeline->SetShader( Shader::PixelShader, g_pForwardPlusPixelShader );
        g_pForwardPlusTransparentPipeline->SetRenderTarget( renderWindow.GetRenderTarget() );
        DepthStencilState::DepthMode depthMode( true, DepthStencilState::DepthWrite::Disable );
        depthMode.DepthFunction = DepthStencilState::CompareFunction::LessOrEqual; // We need to set depth mode to <= because we have a depth prepass in Forward+
        g_pForwardPlusTransparentPipeline->GetDepthStencilState().SetDepthMode( depthMode );
        g_pForwardPlusTransparentPipeline->GetBlendState().SetBlendMode( alphaBlending );
        g_pForwardPlusTransparentPipeline->GetRasterizerState().SetCullMode( RasterizerState::CullMode::None );
    }

    // Create some geometry to render the lights.
    // A sphere is used to render point lights.
    g_Sphere = renderDevice.CreateSphere( 1.0f );
    // Create an inverted cone that is aligned to the z axis.
    // A cone is used to render spotlights.
    g_Cone = renderDevice.CreateCylinder( 0.0f, 1.0f, 1.0f, glm::vec3( 0, 0, 1 ) );
    // An arrow is used to visualize directional lights (for debugging only).
    g_Arrow = renderDevice.CreateArrow( glm::vec3( 0 ), glm::vec3( 0, 0, 1 ), 0.05f );
    // Used for debugging lighting.
    g_Plane = renderDevice.CreatePlane( 100.0f );
    // Used to represent the pivot point of the camera in the scene.
    g_Axis = renderDevice.CreateAxis( 0.01f, 0.1f );

    // Setup forward rendering technique

    // Add a pass to render opaque geometry.
    g_ForwardTechnique.AddPass( std::make_shared<ClearRenderTargetPass>( renderWindow.GetRenderTarget(), ClearFlags::All, g_ClearColor, 1.0f, 0 ) );
    g_ForwardTechnique.AddPass( std::make_shared<BeginQueryPass>( g_pForwardOpaqueQuery ) );
    g_ForwardTechnique.AddPass( std::make_shared<OpaquePass>( g_pScene, g_pOpaquePipeline ) );
    g_ForwardTechnique.AddPass( std::make_shared<EndQueryPass>( g_pForwardOpaqueQuery ) );
    // Add a pass to render a 6-point axis in the scene to visualize the camera's pivot point.
    g_PivotPointPass = std::make_shared<OpaquePass>( g_Axis, g_pUnlitPipeline );
    g_ForwardTechnique.AddPass( g_PivotPointPass );

    // Add a pass for rendering transparent geometry
    g_ForwardTechnique.AddPass( std::make_shared<BeginQueryPass>( g_pForwardTransparentQuery ) );
    g_TransparentPass = std::make_shared<TransparentPass>( g_pScene, g_pTransparentPipeline );
    g_ForwardTechnique.AddPass( g_TransparentPass ); 
    g_ForwardTechnique.AddPass( std::make_shared<EndQueryPass>( g_pForwardTransparentQuery ) );

    // Add a pass to render the lights in the scene as opaque geometry. Can be toggled with 'l' key.
    g_LightsPassFront = std::make_shared<LightsPass>( g_Config.Lights, g_Sphere, g_Cone, g_Arrow, g_pLightsPipelineFront );
    g_LightsPassBack = std::make_shared<LightsPass>( g_Config.Lights, g_Sphere, g_Cone, g_Arrow, g_pLightsPipelineBack );
    g_ForwardTechnique.AddPass( g_LightsPassBack );
    g_ForwardTechnique.AddPass( g_LightsPassFront );

    // Setup deferred rendering technique.
    g_DeferredTechnique.AddPass( std::make_shared<ClearRenderTargetPass>( g_pGBufferRenderTarget, ClearFlags::All, g_ClearColor, 1.0f, 0 ) );
    g_DeferredTechnique.AddPass( std::make_shared<BeginQueryPass>( g_pDeferredGeometryQuery ) );
    g_DeferredTechnique.AddPass( std::make_shared<OpaquePass>( g_pScene, g_pGeometryPipeline ) );
//    g_DeferredTechnique.AddPass( std::make_shared<GenerateMipMapPass>( g_pGBufferRenderTarget ) );
    g_DeferredTechnique.AddPass( std::make_shared<EndQueryPass>( g_pDeferredGeometryQuery ) );

    std::shared_ptr<Texture> depthStencilBuffer = renderWindow.GetRenderTarget()->GetTexture( RenderTarget::AttachmentPoint::DepthStencil );
    g_DeferredTechnique.AddPass( std::make_shared<CopyTexturePass>( depthStencilBuffer, depthStencilTexture ) );
    g_DeferredTechnique.AddPass( std::make_shared<BeginQueryPass>( g_pDeferredLightingQuery ) );
    g_DeferredTechnique.AddPass( std::make_shared<DeferredLightingPass>( g_Config.Lights, g_Sphere, g_Cone, g_pDeferredLightingPipeline1, g_pDeferredLightingPipeline2, g_pDirectionalLightsPipeline, diffuseTexture, specularTexture, normalTexture, depthStencilTexture ) );
    g_DeferredTechnique.AddPass( std::make_shared<EndQueryPass>( g_pDeferredLightingQuery ) );

    g_DeferredTechnique.AddPass( g_PivotPointPass );
    g_DeferredTechnique.AddPass( std::make_shared<BeginQueryPass>( g_pDeferredTransparentQuery ) );
    g_DeferredTechnique.AddPass( g_TransparentPass );
    g_DeferredTechnique.AddPass( std::make_shared<EndQueryPass>( g_pDeferredTransparentQuery ) );

    g_DeferredTechnique.AddPass( g_LightsPassBack );
    g_DeferredTechnique.AddPass( g_LightsPassFront );

    // Add passes for rendering G buffer textures to the screen
    // Orthographic projection matrix for a 1920x1080 screen resolution (Full HD).
    glm::mat4 orthographicProjection = glm::ortho<float>( 0, 1920, 1080, 0 );

    std::shared_ptr<Scene> debugTextureScene = renderDevice.CreateScreenQuad( 20, 475, 1060, 815 );
    g_DebugTexture0Pass = std::make_shared<PostprocessPass>( debugTextureScene, g_pDebugTexturePipeline, orthographicProjection, diffuseTexture );
    g_DebugTexture0Pass->SetEnabled( false ); // Initially disabled. Enabled with the F1 key.
    g_DeferredTechnique.AddPass( g_DebugTexture0Pass );

    debugTextureScene = renderDevice.CreateScreenQuad( 495, 950, 1060, 815 );
    g_DebugTexture1Pass = std::make_shared<PostprocessPass>( debugTextureScene, g_pDebugTexturePipeline, orthographicProjection, specularTexture );
    g_DebugTexture1Pass->SetEnabled( false ); // Initial disabled. Enabled with the F2 key.
    g_DeferredTechnique.AddPass( g_DebugTexture1Pass );

    debugTextureScene = renderDevice.CreateScreenQuad( 970, 1425, 1060, 815 );
    g_DebugTexture2Pass = std::make_shared<PostprocessPass>( debugTextureScene, g_pDebugTexturePipeline, orthographicProjection, normalTexture );
    g_DebugTexture2Pass->SetEnabled( false ); // Initially disabled. Enabled with the F3 key.
    g_DeferredTechnique.AddPass( g_DebugTexture2Pass );

    debugTextureScene = renderDevice.CreateScreenQuad( 1445, 1900, 1060, 815 );
    g_DebugTexture3Pass = std::make_shared<PostprocessPass>( debugTextureScene, g_pDebugDepthTexturePipeline, orthographicProjection, depthStencilTexture );
    g_DebugTexture3Pass->SetEnabled( false ); // Initially disabled. Enabled with the F4 key.
    g_DeferredTechnique.AddPass( g_DebugTexture3Pass );

    // Setup Forward+ rendering technique.

    // Clear the render target.
    g_ForwardPlusTechnique.AddPass( std::make_shared<ClearRenderTargetPass>( renderWindow.GetRenderTarget(), ClearFlags::All, g_ClearColor, 1.0f, 0 ) );
    // Depth pre-pass.
    g_ForwardPlusTechnique.AddPass( std::make_shared<BeginQueryPass>( g_pForwardPlusDepthPrepassQuery ) );
    g_ForwardPlusTechnique.AddPass( std::make_shared<OpaquePass>( g_pScene, g_pDepthPrepassPipeline ) );
    g_ForwardPlusTechnique.AddPass( std::make_shared<EndQueryPass>( g_pForwardPlusDepthPrepassQuery ) );

    g_pLightCullingComputeShader->GetShaderParameterByName( "DepthTextureVS" ).Set( depthStencilBuffer );
    Texture::TextureFormat lightCullingDebugTextureFormat( Texture::Components::RGBA,
                                                           Texture::Type::Float,
                                                           1,
                                                           32, 32, 32, 32, 0, 0 );
    g_pLightCullingDebugTexture = renderDevice.CreateTexture2D( g_Config.WindowWidth, g_Config.WindowHeight, 1, lightCullingDebugTextureFormat, CPUAccess::None, true );
    g_pLightCullingComputeShader->GetShaderParameterByName( "DebugTexture" ).Set( g_pLightCullingDebugTexture );
    g_pLightCullingHeatMap = renderDevice.CreateTexture( L"../Assets/textures/LightCountHeatMap.psd" );
    g_pLightCullingComputeShader->GetShaderParameterByName( "LightCountHeatMap" ).Set( g_pLightCullingHeatMap );
    
    // Will be mapped to the "DispatchParams" in the Forward+ compute shaders.
    g_pDispatchParamsConstantBuffer = renderDevice.CreateConstantBuffer( DispatchParams() );
    // Will be mapped to the "ScreenToViewParams" in the CommonInclude.hlsl shader.
    g_pScreenToViewParamsConstantBuffer = renderDevice.CreateConstantBuffer( ScreenToViewParams() );

    // Light culling pass
    
    // Light list index counter (initial value buffer - required to reset the light list index counters back to 0)
    uint32_t lightListIndexCounterInitialValue = 0;
    std::shared_ptr<StructuredBuffer> lightListIndexCounterInitialBuffer = renderDevice.CreateStructuredBuffer( &lightListIndexCounterInitialValue, 1, sizeof( uint32_t ) );

    // This one will be used as a RWStructuredBuffer in the compute shader.
    g_pLightListIndexCounterOpaque = renderDevice.CreateStructuredBuffer( &lightListIndexCounterInitialValue, 1, sizeof( uint32_t ), CPUAccess::None, true );
    g_pLightListIndexCounterTransparent = renderDevice.CreateStructuredBuffer( &lightListIndexCounterInitialValue, 1, sizeof( uint32_t ), CPUAccess::None, true );

    g_pLightCullingComputeShader->GetShaderParameterByName( "o_LightIndexCounter" ).Set( g_pLightListIndexCounterOpaque );
    g_pLightCullingComputeShader->GetShaderParameterByName( "t_LightIndexCounter" ).Set( g_pLightListIndexCounterTransparent );

    // Reset the light list index counters back to 0.
    g_ForwardPlusTechnique.AddPass( std::make_shared<BeginQueryPass>( g_pForwardPlusLightCullingQuery ) );
    g_ForwardPlusTechnique.AddPass( std::make_shared<CopyBufferPass>( g_pLightListIndexCounterOpaque, lightListIndexCounterInitialBuffer ) );
    g_ForwardPlusTechnique.AddPass( std::make_shared<CopyBufferPass>( g_pLightListIndexCounterTransparent, lightListIndexCounterInitialBuffer ) );

    g_LightCullingDispatchPass = std::make_shared<DispatchPass>( g_pLightCullingComputeShader, glm::ceil( glm::vec3( g_WindowWidth / (float)g_LightCullingBlockSize, g_WindowHeight / (float)g_LightCullingBlockSize, 1 ) ) );
    g_ForwardPlusTechnique.AddPass( g_LightCullingDispatchPass );
    g_ForwardPlusTechnique.AddPass( std::make_shared<EndQueryPass>( g_pForwardPlusLightCullingQuery ) );

    // Forward+ opaque pass.
    g_ForwardPlusTechnique.AddPass( std::make_shared<InvokeFunctionPass>( [=] ()
    {
        // Make sure the pixel shader has the right parameters set before executing the opaque pass.
        g_pForwardPlusPixelShader->GetShaderParameterByName( "LightIndexList" ).Set( g_pLightIndexListOpaque );
        g_pForwardPlusPixelShader->GetShaderParameterByName( "LightGrid" ).Set( g_pLightGridOpaque );
    }
    ) );
    g_ForwardPlusTechnique.AddPass( std::make_shared<BeginQueryPass>( g_pForwardPlusOpaqueQuery ) );
    g_ForwardPlusTechnique.AddPass( std::make_shared<OpaquePass>( g_pScene, g_pForwardPlusOpaquePipeline ) );
    g_ForwardPlusTechnique.AddPass( std::make_shared<EndQueryPass>( g_pForwardPlusOpaqueQuery ) );
    g_ForwardPlusTechnique.AddPass( g_PivotPointPass );

    // Forward+ transparent pass.
    g_ForwardPlusTechnique.AddPass( std::make_shared<InvokeFunctionPass>( [=] ()
    {
        // Make sure the pixel shader has the right parameters set before executing the transparent pass.
        g_pForwardPlusPixelShader->GetShaderParameterByName( "LightIndexList" ).Set( g_pLightIndexListTransparent );
        g_pForwardPlusPixelShader->GetShaderParameterByName( "LightGrid" ).Set( g_pLightGridTransparent );
    }
    ) );
    g_ForwardPlusTechnique.AddPass( std::make_shared<BeginQueryPass>( g_pForwardPlusTransparentQuery ) );
    g_ForwardPlusTechnique.AddPass( std::make_shared<TransparentPass>( g_pScene, g_pForwardPlusTransparentPipeline ) );
    g_ForwardPlusTechnique.AddPass( std::make_shared<EndQueryPass>( g_pForwardPlusTransparentQuery ) );

    g_ForwardPlusTechnique.AddPass( g_LightsPassBack );
    g_ForwardPlusTechnique.AddPass( g_LightsPassFront );

    // Show the depth buffer after light culling compute shader (for debugging)
    debugTextureScene = renderDevice.CreateScreenQuad( 0, 1920, 1080, 0 );
    g_ForwardPlusDebugPass = std::make_shared<PostprocessPass>( debugTextureScene, g_pDebugTextureWithBlendingPipeline, orthographicProjection, g_pLightCullingDebugTexture );
    g_ForwardPlusDebugPass->SetEnabled( false );
    g_ForwardPlusTechnique.AddPass( g_ForwardPlusDebugPass );

    // Setup light picking technique
    g_LightPickingTechnique.AddPass( std::make_shared<ClearRenderTargetPass>( lightPickingTexure, ClearFlags::Color, glm::vec4( 0 ) ) );
    // Copy the depth/stencil values from the render window depth/stencil buffer to the light picking depth stencil buffer.
    // I don't want to be able to pick lights through geometry, but I also need to update the depth buffer so that I will always pick the
    // closest light.  In order to avoid writing to the default depth buffer for light picking, just copy the texture.
    g_LightPickingTechnique.AddPass( std::make_shared<CopyTexturePass>( lightPickingDepthStencilTexture, renderWindow.GetRenderTarget()->GetTexture( RenderTarget::AttachmentPoint::DepthStencil ) ) );
    g_LightPickingTechnique.AddPass( std::make_shared<LightPickingPass>( g_Config.Lights, g_Sphere, g_Cone, g_Arrow, g_pLightPickingPipeline ) );
    // Now copy the resulting texture to the light picking staging texture so it can be read on the CPU.
    g_LightPickingTechnique.AddPass( std::make_shared<CopyTexturePass>( g_LightPickingTexture, lightPickingTexure ) );

    // Create samplers
    g_LinearRepeatSampler = renderDevice.CreateSamplerState();
    g_LinearClampSampler = renderDevice.CreateSamplerState();

    g_LinearRepeatSampler->SetFilter( SamplerState::MinFilter::MinLinear, SamplerState::MagFilter::MagLinear, SamplerState::MipFilter::MipLinear );
    g_LinearClampSampler->SetFilter( SamplerState::MinFilter::MinLinear, SamplerState::MagFilter::MagLinear, SamplerState::MipFilter::MipLinear );
    g_LinearClampSampler->SetWrapMode( SamplerState::WrapMode::Clamp, SamplerState::WrapMode::Clamp, SamplerState::WrapMode::Clamp );

    // Setup Ant Tweak bars
    CreateAntTweakBar();

    // Update and recompile pixel shaders when the number of lights in the scene changes.
    UpdateNumLights();

    g_pCurrentLight = &g_Config.Lights[0];
    g_pCurrentLight->m_Selected = true;

    // Register callbacks
    g_Application.FileChanged += &OnFileChanged;
    renderWindow.Update += &OnUpdate;
    renderWindow.PreRender += &OnPreRender;
    renderWindow.Render += &OnRender;
    renderWindow.PostRender += &OnPostRender;
    renderWindow.KeyPressed += &OnKeyPressed;
    renderWindow.KeyReleased += &OnKeyReleased;
    renderWindow.MouseButtonPressed += &OnMouseButtonPressed;
    renderWindow.MouseButtonReleased += &OnMouseButtonReleased;
    renderWindow.MouseMoved += &OnMouseMoved;
    renderWindow.MouseWheel += &OnMouseWheel;
    renderWindow.JoystickButtonPressed += &OnJoystickButtonPressed;
    renderWindow.JoystickButtonReleased += &OnJoystickButtonReleased;
    renderWindow.JoystickPOV += &OnJoystickPOV;
    renderWindow.JoystickAxis += &OnJoystickAxis;
    renderWindow.Resize += &OnWindowResize;
    renderWindow.Close += &OnWindowClose;

    // Close the loading window.
    loadingWindow.CloseWindow();

    // Show the main window.
    renderWindow.ShowWindow();

    int result = g_Application.Run();

    // Save configuration settings if there is a version mismatch.
    // This will upgrade the configuration file to the latest version
    // of the class.
    if ( g_Config.UpgradeConfigFile )
    {
        g_Config.Save( configFileName );
    }

    return result;
}

void UpdateLights()
{
    glm::mat4 viewMatrix = g_Camera.GetViewMatrix();

    // Update the viewspace vectors of the light.
    for ( unsigned int i = 0; i < g_Config.Lights.size(); i++ )
    {
        // Update the lights so that their position and direction are in view space.
        Light& light = g_Config.Lights[i];
        light.m_PositionVS = viewMatrix * glm::vec4( light.m_PositionWS.xyz(), 1 );
        light.m_DirectionVS = glm::normalize( viewMatrix * glm::vec4( light.m_DirectionWS.xyz(), 0 ) );
    }
    
    // Update constant buffer data with lights array.
    g_pLightsStructuredBuffer->Set( g_Config.Lights );
}

void AddLight()
{
    Light newLight = Light();
    if ( g_pCurrentLight )
    {
        // Copy the properties from the currently selected light.
        newLight = *g_pCurrentLight;
    }

    newLight.m_PositionWS = glm::vec4( g_Camera.GetPivotPoint(), 1 );
    newLight.m_DirectionWS = g_Camera.GetRotation() * glm::vec4( 0, 0, -1, 0 );
//    newLight.m_Range = g_Camera.GetPivotDistance();

    g_Config.Lights.push_back( newLight );

    // Select the new light.
    SetCurrentLight( static_cast<uint32_t>( g_Config.Lights.size() - 1 ) );

    UpdateNumLights();
}

void RemoveLight( size_t index = -1 )
{
    if ( g_Config.Lights.size() > 1 )
    {
        index = std::min<size_t>( index, g_Config.Lights.size() - 1 );

        g_Config.Lights.erase( g_Config.Lights.begin() + index );

        // Update the currently selected light (if the one that got removed was selected)
        SetCurrentLight( std::min<uint32_t>( g_uiCurrentLightIndex, (uint32_t)( g_Config.Lights.size() - 1 ) ) );

        UpdateNumLights();
    }
}

// Focus on the currently selected light.
void FocusLight()
{
    if ( g_pCurrentLight )
    {
        g_bLightTracksCamera = false;
        g_Camera.SetTranslate( glm::vec3( g_pCurrentLight->m_PositionWS ) );
    }
}

void SetCurrentLight( uint32_t newIndex )
{
    if ( /*g_uiCurrentLightIndex != newIndex &&*/ newIndex < g_Config.Lights.size() )
    {
        g_bLightTracksCamera = false;

        g_uiCurrentLightIndex = newIndex;
        if ( g_pCurrentLight )
        {
            g_pCurrentLight->m_Selected = false;
        }

        g_pCurrentLight = &g_Config.Lights[g_uiCurrentLightIndex];

        g_pCurrentLight->m_Selected = true;

        TwRefreshBar( g_pLightsTweakBar );
    }
}

void SelectNextLight()
{
    g_bLightTracksCamera = false;

    // Select the next light in the lights array.
    SetCurrentLight( ( g_uiCurrentLightIndex + 1 ) % g_Config.Lights.size() );

    FocusLight();
}

void SelectPreviousLight()
{
    g_bLightTracksCamera = false;

    // Wrap around to the previous light in the lights array.
    SetCurrentLight( ( g_uiCurrentLightIndex + (uint32_t)( g_Config.Lights.size() - 1 ) ) % g_Config.Lights.size() );

    FocusLight();
}

void ResetStatistics()
{
    g_RunningTime = 0.0;
    g_FrameCount = 0;

    g_FrameStatistic.Reset();

    g_ForwardOpaqueStatistic.Reset();
    g_ForwardTransparentStatistic.Reset();

    g_DeferredGeometryStatistic.Reset();
    g_DeferredLightingStatistic.Reset();
    g_DeferredTransparentStatistic.Reset();

    g_ForwardPlusDepthPrepassStatistic.Reset();
    g_ForwardPlusLightCullingStatistic.Reset();
    g_ForwardPlusOpaqueStatistic.Reset();
    g_ForwardPlusTransparentStatistic.Reset();
}

void UpdateNumLights()
{
    size_t numLights = g_Config.Lights.size();

    RenderDevice& renderDevice = g_Application.GetRenderDevice();

    // Destroy the old constant buffer
    renderDevice.DestroyStructuredBuffer( g_pLightsStructuredBuffer );

    // Create a new one of the right size.
    g_pLightsStructuredBuffer = renderDevice.CreateStructuredBuffer( g_Config.Lights, CPUAccess::Write );

    // Recompile the shaders with the new size of lights array.
    Shader::ShaderMacros shaderMacros;
    {
        std::stringstream ss;
        ss << numLights;
        shaderMacros["NUM_LIGHTS"] = ss.str();
    }

#if defined(_DEBUG)
    std::stringstream debugString;
    debugString << "Number of lights: " << numLights << std::endl;
    OutputDebugStringA( debugString.str().c_str() );
#endif

    // Recompile pixel shaders with updated number of lights.
    g_pPixelShader->LoadShaderFromFile( Shader::PixelShader, L"../Assets/shaders/ForwardRendering.hlsl", shaderMacros, "PS_main", "latest" );
    g_pDeferredLightingPixelShader->LoadShaderFromFile( Shader::PixelShader, L"../Assets/shaders/DeferredRendering.hlsl", shaderMacros, "PS_DeferredLighting", "latest" );

    // Update macros for light culling compute shaders as well.
    SetThreadGroupBlockSize( g_LightCullingBlockSize );
    
    ResetStatistics();

    {
        std::stringstream ss;
        ss << "'Selected Light'/LightIndex max=" << ( numLights - 1 );
        // Update tweak bar min/max values.
        TwDefine( ss.str().c_str() );
        TwRefreshBar( g_pLightsTweakBar );
    }
}

void GenerateLights( LightGeneration genMethod, uint32_t numLights )
{
    if ( g_pCurrentLight )
    {
        g_pCurrentLight->m_Selected = false;
        g_pCurrentLight = nullptr;
    }

    g_Config.Lights.resize( numLights );

    uint32_t lightsPerDimension = static_cast<uint32_t>( std::ceil( std::cbrt( static_cast<float>(numLights) ) ) );
    glm::vec3 bounds = g_Config.LightsMaxBounds - g_Config.LightsMinBounds;

    for ( uint32_t i = 0; i < numLights; i++ )
    {
        Light& light = g_Config.Lights[i];

        switch ( genMethod )
        {
        case LightGeneration::Uniform:
        {
            glm::vec3 pos;
            pos.x = ( i % lightsPerDimension ) / static_cast<float>( lightsPerDimension );
            pos.y = ( static_cast<uint32_t>( std::floor( i / static_cast<float>( lightsPerDimension ) ) ) % lightsPerDimension ) / static_cast<float>( lightsPerDimension );
            pos.z = ( static_cast<uint32_t>( std::floor( i / static_cast<float>( lightsPerDimension ) / static_cast<float>( lightsPerDimension ) ) ) % lightsPerDimension ) / static_cast<float>( lightsPerDimension );

            light.m_PositionWS = glm::vec4( pos * bounds + g_Config.LightsMinBounds, 1.0f );
        }
        break;
        case LightGeneration::Random:
            light.m_PositionWS = glm::vec4( glm::linearRand( g_Config.LightsMinBounds, g_Config.LightsMaxBounds ), 1.0f );
            break;
        default:
            break;
        }

        // Choose a color that will never be black.
        glm::vec2 colorWheel = glm::diskRand( 1.0f );
        float radius = glm::length( colorWheel );
        light.m_Color.rgb = glm::lerp(
            glm::lerp(
                glm::lerp( glm::vec3( 1 ), glm::vec3( 0, 1, 0 ), radius ),
                glm::lerp( glm::vec3( 1 ), glm::vec3( 1, 0, 0 ), radius ),
                colorWheel.x * 0.5f + 0.5f ),
            glm::lerp(
                glm::lerp( glm::vec3( 1 ), glm::vec3( 0, 0, 1 ), radius ),
                glm::lerp( glm::vec3( 1 ), glm::vec3( 1, 1, 0 ), radius ),
                colorWheel.y * 0.5f + 0.5f ),
            glm::abs( colorWheel.y ) );

        light.m_DirectionWS = glm::vec4( glm::sphericalRand( 1.0f ), 0.0f );
        light.m_Range = glm::linearRand( g_Config.MinRange, g_Config.MaxRange );
        light.m_SpotlightAngle = glm::linearRand( g_Config.MinSpotAngle, g_Config.MaxSpotAngle );

        float fLightPropability = glm::linearRand( 0.0f, 1.0f );

        if ( g_Config.GeneratePointLights && g_Config.GenerateSpotLights && g_Config.GenerateDirectionalLights )
        {
            light.m_Type = ( fLightPropability < 0.33f ? Light::LightType::Point : fLightPropability < 0.66f ? Light::LightType::Spot : Light::LightType::Directional );
        }
        else if ( g_Config.GeneratePointLights && g_Config.GenerateSpotLights && !g_Config.GenerateDirectionalLights )
        {
            light.m_Type = ( fLightPropability < 0.5f ? Light::LightType::Point : Light::LightType::Spot );
        }
        else if ( g_Config.GeneratePointLights && !g_Config.GenerateSpotLights && g_Config.GenerateDirectionalLights )
        {
            light.m_Type = ( fLightPropability < 0.5f ? Light::LightType::Point : Light::LightType::Directional );
        }
        else if ( g_Config.GeneratePointLights && !g_Config.GenerateSpotLights && !g_Config.GenerateDirectionalLights )
        {
            light.m_Type = Light::LightType::Point;
        }
        else if ( !g_Config.GeneratePointLights && g_Config.GenerateSpotLights && g_Config.GenerateDirectionalLights )
        {
            light.m_Type = ( fLightPropability < 0.5f ? Light::LightType::Spot : Light::LightType::Directional );
        }
        else if ( !g_Config.GeneratePointLights && g_Config.GenerateSpotLights && !g_Config.GenerateDirectionalLights )
        {
            light.m_Type = Light::LightType::Spot;
        }
        else if ( !g_Config.GeneratePointLights && !g_Config.GenerateSpotLights && g_Config.GenerateDirectionalLights )
        {
            light.m_Type = Light::LightType::Directional;
        }
        else if ( !g_Config.GeneratePointLights && !g_Config.GenerateSpotLights && !g_Config.GenerateDirectionalLights )
        {
            light.m_Type = ( fLightPropability < 0.33f ? Light::LightType::Point : fLightPropability < 0.66f ? Light::LightType::Spot : Light::LightType::Directional );
        }
    }

    // Make sure a light is selected for ANTweak bar.
    SetCurrentLight( 0 );

    UpdateNumLights();
}


void UpdateGridFrustums()
{
    RenderDevice& renderDevice = g_Application.GetRenderDevice();

    // Make sure we can create at least 1 thread (even if the window is minimized)
    uint32_t screenWidth = std::max( g_WindowWidth, 1u );
    uint32_t screenHeight = std::max( g_WindowHeight, 1u );

    // To compute the frustums for the grid tiles, each thread will compute a single 
    // frustum for the tile.
    glm::uvec3 numThreads = glm::ceil( glm::vec3( screenWidth / (float)g_LightCullingBlockSize, screenHeight / (float)g_LightCullingBlockSize, 1 ) );
    glm::uvec3 numThreadGroups = glm::ceil( glm::vec3( numThreads.x / (float)g_LightCullingBlockSize, numThreads.y / (float)g_LightCullingBlockSize, 1 ) );

    // Update the number of thread groups for the compute frustums compute shader.
    DispatchParams dispatchParams;
    dispatchParams.m_NumThreadGroups = numThreadGroups;
    dispatchParams.m_NumThreads = numThreads;
    g_pDispatchParamsConstantBuffer->Set( dispatchParams );

    // Destroy the previous structured buffer for storing gird frustums.
    renderDevice.DestroyStructuredBuffer( g_pGridFrustums );

    // Create a new RWStructuredBuffer for storing the grid frustums.
    // We need 1 frustum for each grid cell.
    // For 1280x720 screen resolution and 16x16 tile size, results in 80x45 grid 
    // for a total of 3,600 frustums.
    g_pGridFrustums = renderDevice.CreateStructuredBuffer( nullptr, numThreads.x * numThreads.y * numThreads.z,
                                                           sizeof( Frustum ), CPUAccess::None, true );

    // Dispatch the compute shader to recompute the grid frustums.
    g_pComputeFrustumsComputeShader->GetShaderParameterByName( "DispatchParams" ).Set( g_pDispatchParamsConstantBuffer );
    g_pComputeFrustumsComputeShader->GetShaderParameterByName( "ScreenToViewParams" ).Set( g_pScreenToViewParamsConstantBuffer );
    g_pComputeFrustumsComputeShader->GetShaderParameterByName( "out_Frustums" ).Set( g_pGridFrustums );

    g_pComputeFrustumsComputeShader->Bind();
    g_pComputeFrustumsComputeShader->Dispatch( numThreadGroups );
    g_pComputeFrustumsComputeShader->UnBind();

    // Update the light culling compute shader with the computed grid frustums StructuredBuffer.
    g_pLightCullingComputeShader->GetShaderParameterByName( "in_Frustums" ).Set( g_pGridFrustums );
}

void SetThreadGroupBlockSize( uint16_t blockSize )
{
    RenderDevice& renderDevice = g_Application.GetRenderDevice();

    g_LightCullingBlockSize = blockSize;
    size_t numLights = g_Config.Lights.size();

    // Recompile the compute shader with the updated macros.
    Shader::ShaderMacros shaderMacros;
    {
        std::stringstream ss;
        ss << numLights;
        shaderMacros["NUM_LIGHTS"] = ss.str();

        ss.str( std::string() );
        ss.clear();
        ss << g_LightCullingBlockSize;
        shaderMacros["BLOCK_SIZE"] = ss.str();
    }

    g_pLightCullingComputeShader->LoadShaderFromFile( Shader::ComputeShader, L"../Assets/shaders/ForwardPlusRendering.hlsl", shaderMacros, "CS_main", "cs_5_0" );
    g_pComputeFrustumsComputeShader->LoadShaderFromFile( Shader::ComputeShader, L"../Assets/shaders/ForwardPlusRendering.hlsl", shaderMacros, "CS_ComputeFrustums", "cs_5_0" );
    g_pForwardPlusPixelShader->LoadShaderFromFile( Shader::PixelShader, L"../Assets/shaders/ForwardPlusRendering.hlsl", shaderMacros, "PS_main", "latest" );

    // Recompute the frustums for the grid.
    UpdateGridFrustums();

    uint32_t screenWidth = std::max( g_WindowWidth, 1u );
    uint32_t screenHeight = std::max( g_WindowHeight, 1u );

    glm::uvec3 numThreadGroups = glm::ceil( glm::vec3( screenWidth / (float)g_LightCullingBlockSize, screenHeight / (float)g_LightCullingBlockSize, 1 ) );

    // Update the number of thread groups for the light culling compute shader.
    g_LightCullingDispatchPass->SetNumGroups( numThreadGroups );

    // Update the dispatch params for the light culling compute shader.
    DispatchParams dispatchParams;
    dispatchParams.m_NumThreadGroups = numThreadGroups;
    dispatchParams.m_NumThreads = numThreadGroups * glm::uvec3( g_LightCullingBlockSize, g_LightCullingBlockSize, 1 );
    g_pDispatchParamsConstantBuffer->Set( dispatchParams );
    g_pLightCullingComputeShader->GetShaderParameterByName( "DispatchParams" ).Set( g_pDispatchParamsConstantBuffer );

    // Update the light index list.
    // Destroy the previous light index list.
    renderDevice.DestroyStructuredBuffer( g_pLightIndexListOpaque );
    renderDevice.DestroyStructuredBuffer( g_pLightIndexListTransparent );
    // Create a new one to match the required dimensions.
    g_pLightIndexListOpaque = renderDevice.CreateStructuredBuffer( nullptr, numThreadGroups.x * numThreadGroups.y * numThreadGroups.z * AVERAGE_OVERLAPPING_LIGHTS_PER_TILE,
                                                             sizeof( uint32_t ), CPUAccess::None, true );
    g_pLightIndexListTransparent = renderDevice.CreateStructuredBuffer( nullptr, numThreadGroups.x * numThreadGroups.y * numThreadGroups.z * AVERAGE_OVERLAPPING_LIGHTS_PER_TILE,
                                                                        sizeof( uint32_t ), CPUAccess::None, true );

    g_pLightCullingComputeShader->GetShaderParameterByName( "o_LightIndexList" ).Set( g_pLightIndexListOpaque );
    g_pLightCullingComputeShader->GetShaderParameterByName( "t_LightIndexList" ).Set( g_pLightIndexListTransparent );

    // Update the light grid
    // Destroy the old light grid.
    renderDevice.DestroyTexture( g_pLightGridOpaque );
    renderDevice.DestroyTexture( g_pLightGridTransparent );

    // Create a new one to match the required dimensions.
    Texture::TextureFormat lightGridFormat( Texture::Components::RG,
                                            Texture::Type::UnsignedInteger,
                                            1,
                                            32, 32, 0, 0, 0, 0
                                            );
    g_pLightGridOpaque = renderDevice.CreateTexture2D( numThreadGroups.x, numThreadGroups.y, numThreadGroups.z, lightGridFormat, CPUAccess::None, true );
    g_pLightGridTransparent = renderDevice.CreateTexture2D( numThreadGroups.x, numThreadGroups.y, numThreadGroups.z, lightGridFormat, CPUAccess::None, true );

    g_pLightCullingComputeShader->GetShaderParameterByName( "o_LightGrid" ).Set( g_pLightGridOpaque );
    g_pLightCullingComputeShader->GetShaderParameterByName( "t_LightGrid" ).Set( g_pLightGridTransparent );

    ResetStatistics();
}

void OnUpdate( UpdateEventArgs& e )
{
    g_RunningTime += e.ElapsedTime;

    float moveMultiplier = ( g_CameraMovement.TranslateFaster ) ? g_Config.FastCameraSpeed : g_Config.NormalCameraSpeed;
    float rotateMultiplier = ( g_CameraMovement.RotateFaster ) ? g_Config.FastCameraSpeed : g_Config.NormalCameraSpeed;

    g_Camera.TranslateX( ( g_CameraMovement.Right - g_CameraMovement.Left ) * e.ElapsedTime * moveMultiplier );
    g_Camera.TranslateY( ( g_CameraMovement.Up - g_CameraMovement.Down ) * e.ElapsedTime * moveMultiplier );
    g_Camera.TranslateZ( ( g_CameraMovement.Back - g_CameraMovement.Forward ) * e.ElapsedTime * moveMultiplier );
    g_Camera.AddPitch( g_CameraMovement.Pitch * 60.0f * e.ElapsedTime * rotateMultiplier, Camera::Space::Local );
    g_Camera.AddYaw( g_CameraMovement.Yaw * 60.0f * e.ElapsedTime * rotateMultiplier, Camera::Space::World );
    //g_Camera.AddRoll( ( g_CameraMovement.RollCW - g_CameraMovement.RollCCW ) * e.ElapsedTime * 45.0f );

    float fPivot = g_Camera.GetPivotDistance();
    fPivot += g_CameraMovement.PivotTranslate * e.ElapsedTime * moveMultiplier;
    g_Camera.SetPivotDistance( fPivot );

    if ( g_Animate )
    {
        float fRotation = e.ElapsedTime * glm::half_pi<float>();
        glm::mat4 rot = glm::rotate( glm::mat4(1), fRotation, glm::vec3( 0, 1, 0 ) );
        for ( unsigned int i = 0; i < g_Config.Lights.size(); i++ )
        {
            Light& light = g_Config.Lights[i];
            light.m_PositionWS = rot * light.m_PositionWS;
            light.m_DirectionWS = rot * light.m_DirectionWS;
        }
    }

    // Move the currently selected light with the camera.
    if ( g_pCurrentLight && g_bLightTracksCamera )
    {
        g_pCurrentLight->m_PositionWS = glm::vec4( g_Camera.GetPivotPoint(), 1 );
        g_pCurrentLight->m_DirectionWS = g_Camera.GetRotation() * glm::vec4( 0, 0, -1, 0 );
        //g_pCurrentLight->m_Range = g_Camera.GetPivotDistance();
    }

    UpdateLights();
}

void OnPreRender( RenderEventArgs& e )
{
    if ( g_bResizePending )
    {
        ResizeBuffers( g_WindowWidth, g_WindowHeight );
        g_bResizePending = false;
    }

    g_pFrameQuery->Begin( e.FrameCounter );

    // Bind the lights constant buffer to the constant buffer slot in the pixel/compute shaders.
    g_pPixelShader->GetShaderParameterByName( "Lights" ).Set( g_pLightsStructuredBuffer );
    g_pDeferredLightingPixelShader->GetShaderParameterByName( "Lights" ).Set( g_pLightsStructuredBuffer );
    g_pForwardPlusPixelShader->GetShaderParameterByName( "Lights" ).Set( g_pLightsStructuredBuffer );
    g_pLightCullingComputeShader->GetShaderParameterByName( "Lights" ).Set( g_pLightsStructuredBuffer );
    g_pLightCullingComputeShader->GetShaderParameterByName( "ScreenToViewParams" ).Set( g_pScreenToViewParamsConstantBuffer );

    // Bind sampler states to shaders.
    g_pPixelShader->GetShaderParameterByName( "LinearRepeatSampler" ).Set( g_LinearRepeatSampler );
    g_pPixelShader->GetShaderParameterByName( "LinearClampSampler" ).Set( g_LinearClampSampler );
    g_pUnlitPixelShader->GetShaderParameterByName( "LinearRepeatSampler" ).Set( g_LinearRepeatSampler );
    g_pUnlitPixelShader->GetShaderParameterByName( "LinearClampSampler" ).Set( g_LinearClampSampler );
    g_pGeometryPixelShader->GetShaderParameterByName( "LinearRepeatSampler" ).Set( g_LinearRepeatSampler );
    g_pGeometryPixelShader->GetShaderParameterByName( "LinearClampSampler" ).Set( g_LinearClampSampler );
    g_pDebugTexturePixelShader->GetShaderParameterByName( "LinearRepeatSampler" ).Set( g_LinearRepeatSampler );
    g_pDebugTexturePixelShader->GetShaderParameterByName( "LinearClampSampler" ).Set( g_LinearClampSampler );
    g_pDebugDepthTexturePixelShader->GetShaderParameterByName( "LinearRepeatSampler" ).Set( g_LinearRepeatSampler );
    g_pDebugDepthTexturePixelShader->GetShaderParameterByName( "LinearClampSampler" ).Set( g_LinearClampSampler );
    g_pDeferredLightingPixelShader->GetShaderParameterByName( "LinearRepeatSampler" ).Set( g_LinearRepeatSampler );
    g_pDeferredLightingPixelShader->GetShaderParameterByName( "LinearClampSampler" ).Set( g_LinearClampSampler );
    g_pLightCullingComputeShader->GetShaderParameterByName( "LinearRepeatSampler" ).Set( g_LinearRepeatSampler );
    g_pLightCullingComputeShader->GetShaderParameterByName( "LinearClampSampler" ).Set( g_LinearClampSampler );
    g_pForwardPlusPixelShader->GetShaderParameterByName( "LinearRepeatSampler" ).Set( g_LinearRepeatSampler );
    g_pForwardPlusPixelShader->GetShaderParameterByName( "LinearClampSampler" ).Set( g_LinearClampSampler );
}

void OnRender( RenderEventArgs& e )
{
    // Render the scene from the perspective of this camera.
    e.Camera = &g_Camera;

    // Move the 6-point axis to the correct spot
    g_Axis->GetRootNode()->SetLocalTransform( glm::translate( g_Camera.GetPivotPoint() ) );

    // Enable/disable passes
    g_PivotPointPass->SetEnabled( g_Camera.GetPivotDistance() > 0.0f );
    g_LightsPassFront->SetEnabled( g_RenderLights );
    g_LightsPassBack->SetEnabled( g_RenderLights );

    switch ( g_RenderingTechnique )
    {
    case RenderingTechnique::Forward:
        g_ForwardTechnique.Render( e );
        break;
    case RenderingTechnique::Deferred:
        g_DeferredTechnique.Render( e );
        break;
    case RenderingTechnique::ForwardPlus:
        g_ForwardPlusTechnique.Render( e );
        break;
    }
    

    // Generate light picking texture
    // This is only done when the mouse is clicked.
    // @see: OnMouseButtonReleased
    //g_LightPickingTechnique.Render( e );
}

void OnPostRender( RenderEventArgs& e )
{
    g_pFrameQuery->End( e.FrameCounter );
    g_FrameCount += 1;

    RenderWindow& renderWindow = dynamic_cast<RenderWindow&>( const_cast<Object&>( e.Caller ) );
    renderWindow.Present();

    // Retrieve GPU timer results.
    // Don't retrieve the immediate query result, but from the previous frame.
    // Checking previous frame counters will alleviate GPU stalls.
    Query::QueryResult frameResult = g_pFrameQuery->GetQueryResult( e.FrameCounter - ( g_pFrameQuery->GetBufferCount() - 1 ) );
    if ( frameResult.IsValid )
    {
        // Frame time in milliseconds
        g_FrameTime = frameResult.ElapsedTime * 1000.0;
        g_FrameStatistic.Sample( g_FrameTime );
    }

    switch ( g_RenderingTechnique )
    {
    case RenderingTechnique::Forward:
    {
        // Query results for forward rendering technique.
        Query::QueryResult forwardOpaqueResult = g_pForwardOpaqueQuery->GetQueryResult( e.FrameCounter - ( g_pForwardOpaqueQuery->GetBufferCount() - 1 ) );
        Query::QueryResult forwardTransparentResult = g_pForwardTransparentQuery->GetQueryResult( e.FrameCounter - ( g_pForwardTransparentQuery->GetBufferCount() - 1 ) );

        if ( forwardOpaqueResult.IsValid )
        {
            g_ForwardOpaqueStatistic.Sample( forwardOpaqueResult.ElapsedTime * 1000.0 );
        }
        if ( forwardTransparentResult.IsValid )
        {
            g_ForwardTransparentStatistic.Sample( forwardTransparentResult.ElapsedTime * 1000.0 );
        }
    }
    break;
    case RenderingTechnique::Deferred:
    {
        // Query results from deferred rendering technique.
        Query::QueryResult deferredGeometryResult = g_pDeferredGeometryQuery->GetQueryResult( e.FrameCounter - ( g_pDeferredGeometryQuery->GetBufferCount() - 1 ) );
        Query::QueryResult deferredLightingResult = g_pDeferredLightingQuery->GetQueryResult( e.FrameCounter - ( g_pDeferredLightingQuery->GetBufferCount() - 1 ) );
        Query::QueryResult deferredTransparentResult = g_pDeferredTransparentQuery->GetQueryResult( e.FrameCounter - ( g_pDeferredTransparentQuery->GetBufferCount() - 1 ) );

        if ( deferredGeometryResult.IsValid )
        {
            g_DeferredGeometryStatistic.Sample( deferredGeometryResult.ElapsedTime * 1000.0 );
        }
        if ( deferredLightingResult.IsValid )
        {
            g_DeferredLightingStatistic.Sample( deferredLightingResult.ElapsedTime * 1000.0 );
        }
        if ( deferredTransparentResult.IsValid )
        {
            g_DeferredTransparentStatistic.Sample( deferredTransparentResult.ElapsedTime * 1000.0 );
        }
    }
    break;
    case RenderingTechnique::ForwardPlus:
    {
        // Query results from Forward+ rendering technique.
        Query::QueryResult forwardPlusDepthPrepassResult = g_pForwardPlusDepthPrepassQuery->GetQueryResult( e.FrameCounter - ( g_pForwardPlusDepthPrepassQuery->GetBufferCount() - 1 ) );
        Query::QueryResult forwardPlusLightCullingResult = g_pForwardPlusLightCullingQuery->GetQueryResult( e.FrameCounter - ( g_pForwardPlusLightCullingQuery->GetBufferCount() - 1 ) );
        Query::QueryResult forwardPlusOpaqueResult = g_pForwardPlusOpaqueQuery->GetQueryResult( e.FrameCounter - ( g_pForwardPlusOpaqueQuery->GetBufferCount() - 1 ) );
        Query::QueryResult forwardPlusTransparentResult = g_pForwardPlusTransparentQuery->GetQueryResult( e.FrameCounter - ( g_pForwardPlusTransparentQuery->GetBufferCount() - 1 ) );

        if ( forwardPlusDepthPrepassResult.IsValid )
        {
            g_ForwardPlusDepthPrepassStatistic.Sample( forwardPlusDepthPrepassResult.ElapsedTime * 1000.0 );
        }
        if ( forwardPlusLightCullingResult.IsValid )
        {
            g_ForwardPlusLightCullingStatistic.Sample( forwardPlusLightCullingResult.ElapsedTime * 1000.0 );
        }
        if ( forwardPlusOpaqueResult.IsValid )
        {
            g_ForwardPlusOpaqueStatistic.Sample( forwardPlusOpaqueResult.ElapsedTime * 1000.0 );
        }
        if ( forwardPlusTransparentResult.IsValid )
        {
            g_ForwardPlusTransparentStatistic.Sample( forwardPlusTransparentResult.ElapsedTime * 1000.0 );
        }
    }
    break;
    }

}

void ResizeBuffers( unsigned int width, unsigned int height )
{
    g_Camera.SetProjectionRH( 45.0f, width / (float)height, 0.1f, 1000.0f );

    Viewport viewport( 0.0f, 0.0f, (float)width, (float)height );

    // Camera needs the viewport dimensions for the pivot camera to work.
    g_Camera.SetViewport( viewport );

    // Update the screen to view params constant buffer.
    // Used to convert screen space coordinates to view space.
    ScreenToViewParams screenToViewParams;
    screenToViewParams.m_InverseProjectionMatrix = glm::inverse( g_Camera.GetProjectionMatrix() );
    screenToViewParams.m_ScreenDimensions = glm::vec2( width, height );

    g_pScreenToViewParamsConstantBuffer->Set( screenToViewParams );

    // Update all the pipeline states with the new viewport dimensions.
    g_pLightsPipelineFront->GetRasterizerState().SetViewport( viewport );
    g_pLightsPipelineBack->GetRasterizerState().SetViewport( viewport );
    g_pLightPickingPipeline->GetRasterizerState().SetViewport( viewport );
    g_pOpaquePipeline->GetRasterizerState().SetViewport( viewport );
    g_pTransparentPipeline->GetRasterizerState().SetViewport( viewport );
    g_pUnlitPipeline->GetRasterizerState().SetViewport( viewport );
    g_pGeometryPipeline->GetRasterizerState().SetViewport( viewport );
    g_pDebugTexturePipeline->GetRasterizerState().SetViewport( viewport );
    g_pDebugTextureWithBlendingPipeline->GetRasterizerState().SetViewport( viewport );
    g_pDebugDepthTexturePipeline->GetRasterizerState().SetViewport( viewport );
    g_pDeferredLightingPipeline1->GetRasterizerState().SetViewport( viewport );
    g_pDeferredLightingPipeline2->GetRasterizerState().SetViewport( viewport );
    g_pDirectionalLightsPipeline->GetRasterizerState().SetViewport( viewport );
    g_pDepthPrepassPipeline->GetRasterizerState().SetViewport( viewport );
    g_pForwardPlusOpaquePipeline->GetRasterizerState().SetViewport( viewport );
    g_pForwardPlusTransparentPipeline->GetRasterizerState().SetViewport( viewport );

    // Update render targets
    g_pGBufferRenderTarget->Resize( width, height);
    g_pDepthOnlyRenderTarget->Resize( width, height );
    g_pColorOnlyRenderTarget->Resize( width, height );
    g_pLightPickingRenderTarget->Resize( width, height);
    g_LightPickingTexture->Resize( width, height );

    g_pLightCullingDebugTexture->Resize( width, height );

    // Update the thread group block size based on the new screen resolution.
    SetThreadGroupBlockSize( g_LightCullingBlockSize );

    ResetStatistics();
}

void OnWindowResize( ResizeEventArgs& e )
{
    e.Width = std::max( e.Width, 1 );
    e.Height = std::max( e.Height, 1 );

    g_WindowWidth = e.Width;
    g_WindowHeight = e.Height;

    // Resize the render targets and textures
    // the next time PreRender is called.
    g_bResizePending = true;

}

void OnKeyPressed( KeyEventArgs& e )
{
    switch ( e.Key )
    {
    case KeyCode::Escape:
    {
        g_Application.Stop();
    }
    break;
    case KeyCode::W:
    {
        g_CameraMovement.Forward = 1.0f;
    }
    break;
    case KeyCode::A:
    {
        g_CameraMovement.Left = 1.0f;
    }
    break;
    case KeyCode::S:
    {
        if ( e.Control )
        {
            // Save the configuration file.
            // Let's us setup camera settings in the configuration settings.
            g_Config.CameraPosition = g_Camera.GetTranslation();
            g_Config.CameraRotation = g_Camera.GetRotation();
            g_Config.CameraPivotDistance = g_Camera.GetPivotDistance();

            g_Config.Save();
        }
        else
        {
            g_CameraMovement.Back = 1.0f;
        }
    }
    break;
    case KeyCode::D:
    {
        g_CameraMovement.Right = 1.0f;
    }
    break;
    case KeyCode::Q:
    {
        g_CameraMovement.Down = 1.0f;
    }
    break;
    case KeyCode::E:
    {
        g_CameraMovement.Up = 1.0f;
    }
    break;
    case KeyCode::F:
    {
        FocusLight();
    }
    break;
    case KeyCode::R:
    {
        if ( e.Control )
        {
            // Reload the previously loaded configuration file.
            // This might be needed if the user changes the configuration while the application is running.
            g_Config.Reload();

            g_Camera.SetTranslate( g_Config.CameraPosition );
            g_Camera.SetRotate( g_Config.CameraRotation );
            g_Camera.SetPivotDistance( g_Config.CameraPivotDistance );
            // Scale the scene to fit the view.
            if ( g_pScene && g_pScene->GetRootNode() )
            {
                g_pScene->GetRootNode()->SetLocalTransform( glm::scale( glm::vec3( g_Config.SceneScaleFactor ) ) );
            }
        }
        else
        {
            g_CameraMovement.RollCW = 1.0f;
        }
    }
    break;
    //case KeyCode::F:
    //{
    //    g_CameraMovement.RollCCW = 1.0f;
    //}
    //break;
    case KeyCode::L:
    {
        g_RenderLights = !g_RenderLights;

        TwRefreshBar( g_pRenderingTechniqueTweakBar );

        if ( g_RenderLights )
        {
            OutputDebugStringA( "Debug lights enabled.\n" );
        }
        else
        {
            OutputDebugStringA( "Debug lights disabled.\n" );
        }

    }
    break;
    case KeyCode::ShiftKey:
    {
        g_CameraMovement.TranslateFaster = true;
        g_CameraMovement.RotateFaster = true;
    }
    break;
    case KeyCode::Add:
    case KeyCode::OemPlus:
    {
        AddLight();
    }
    break;
    case KeyCode::Subtract:
    case KeyCode::OemMinus:
    {
        // Remove the currently selected light.
        RemoveLight(g_uiCurrentLightIndex);
    }
    break;
    case KeyCode::Right:
    {
        SelectNextLight();
    }
    break;
    case KeyCode::Left:
    {
        SelectPreviousLight();
    }
    break;
    case KeyCode::Space:
    {
        g_Animate = !g_Animate;
    }
    break;
    case KeyCode::F5:
    {
        // Reset camera to default values.
        g_Camera.SetTranslate( g_Config.CameraPosition );
        g_Camera.SetRotate( g_Config.CameraRotation );
    }
    break;
    case KeyCode::D0:
    {
        g_Camera.SetTranslate( glm::vec3( 0 ) );
        g_Camera.SetRotate( glm::quat() );
        g_Camera.SetPivotDistance( 0 );
    }
    break;
    case KeyCode::D1:
    {
        g_RenderingTechnique = RenderingTechnique::Forward;
        g_RunningTime = 0.0;
        g_FrameCount = 0;
        g_FrameStatistic.Reset();
        TwRefreshBar( g_pRenderingTechniqueTweakBar );
    }
    break;
    case KeyCode::D2:
    {
        g_RenderingTechnique = RenderingTechnique::Deferred;
        g_RunningTime = 0.0;
        g_FrameCount = 0;
        g_FrameStatistic.Reset();
        TwRefreshBar( g_pRenderingTechniqueTweakBar );
    }
    break;
    case KeyCode::D3:
    {
        g_RenderingTechnique = RenderingTechnique::ForwardPlus;
        g_RunningTime = 0.0;
        g_FrameCount = 0;
        g_FrameStatistic.Reset();
        TwRefreshBar( g_pRenderingTechniqueTweakBar );
    }
    break;
    case KeyCode::F1:
    {
        g_DebugTexture0Pass->SetEnabled( !g_DebugTexture0Pass->IsEnabled() );
        g_ForwardPlusDebugPass->SetEnabled( !g_ForwardPlusDebugPass->IsEnabled() );
    }
    break;
    case KeyCode::F2:
    {
        g_DebugTexture1Pass->SetEnabled( !g_DebugTexture1Pass->IsEnabled() );
    }
    break;
    case KeyCode::F3:
    {
        g_DebugTexture2Pass->SetEnabled( !g_DebugTexture2Pass->IsEnabled() );
    }
    break;
    case KeyCode::F4:
    {
        g_DebugTexture3Pass->SetEnabled( !g_DebugTexture3Pass->IsEnabled() );
    }
    break;
    }

}

void OnKeyReleased( KeyEventArgs& e )
{
    switch ( e.Key )
    {
    case KeyCode::W:
    {
        g_CameraMovement.Forward = 0.0f;
    }
    break;
    case KeyCode::A:
    {
        g_CameraMovement.Left = 0.0f;
    }
    break;
    case KeyCode::S:
    {
        g_CameraMovement.Back = 0.0f;
    }
    break;
    case KeyCode::D:
    {
        g_CameraMovement.Right = 0.0f;
    }
    break;
    case KeyCode::Q:
    {
        g_CameraMovement.Down = 0.0f;
    }
    break;
    case KeyCode::E:
    {
        g_CameraMovement.Up = 0.0f;
    }
    break;
    case KeyCode::R:
    {
        g_CameraMovement.RollCW = 0.0f;
    }
    break;
    case KeyCode::F:
    {
        g_CameraMovement.RollCCW = 0.0f;
    }
    break;
    case KeyCode::ShiftKey:
    {
        g_CameraMovement.TranslateFaster = false;
        g_CameraMovement.RotateFaster = false;
    }
    break;
    }
}

void OnMouseButtonPressed( MouseButtonEventArgs& e )
{
    g_Camera.OnMousePressed( e );

    g_PreviousMousePosition = glm::vec2( e.X, e.Y );
}

void OnMouseButtonReleased( MouseButtonEventArgs& e )
{
    glm::vec2 currentMousePosition = glm::vec2( e.X, e.Y );
    float offset = glm::distance( g_PreviousMousePosition, currentMousePosition );

    // If the mouse moved less than 3 pixels
    if ( offset < 3.0f )
    {
        RenderEventArgs renderEventArgs( e.Caller, 0, 0, 0, &g_Camera, g_pLightPickingPipeline.get() );
        g_LightPickingTechnique.Render( renderEventArgs );

        // Now read the light index under the mouse cursor.
        uint16_t index = g_LightPickingTexture->FetchPixel<uint16_t>( currentMousePosition );

        if ( index > 0 )
        {
            // The index stored in the picking texture is 1-based index.
            // So before we select the light, we need to convert it to the 0-based index 
            // expected by this function..
            SetCurrentLight( index - 1 );
        }
    }
}

void OnMouseMoved( MouseMotionEventArgs& e )
{
    if ( e.LeftButton )
    {
        g_Camera.OnMouseMoved( e );
        //float maxDimension = glm::max<float>( g_WindowWidth, g_WindowHeight );
        //g_Camera.AddPitch( e.RelY / maxDimension * 180.0f );
        //g_Camera.AddYaw( e.RelX / maxDimension * 180.0f );
    }
}

void OnMouseWheel( MouseWheelEventArgs& e )
{
    float fPivot = g_Camera.GetPivotDistance();
    fPivot -= e.WheelDelta * ( g_CameraMovement.TranslateFaster ? 1.0f : 0.1f );
    g_Camera.SetPivotDistance( fPivot );
}

void OnWindowClose( WindowCloseEventArgs& e )
{
    g_Application.Stop();
}

void OnJoystickButtonPressed( JoystickButtonEventArgs& e )
{
    switch ( e.ButtonID )
    {
    case 0: // A on Xbox360 controller.
    case 8: // LS on Xbox360 controller.
        g_CameraMovement.TranslateFaster = true;
        break;
    case 9: // RS on XBox360 controller.
        g_CameraMovement.RotateFaster = true;
        break;
    case 5: // RB on XBox360 controller.
        g_CameraMovement.Up = 1.0f;
        break;
    case 4: // LB on XBox360 controller.
        g_CameraMovement.Down = 1.0f;
        break;
    case 6: // Back on Xbox360 controller.
        g_Application.Stop();
        break;
    case 7: // Start on XBox360 controller.
        FocusLight();
        break;

    }
}

void OnJoystickButtonReleased( JoystickButtonEventArgs& e )
{
    switch ( e.ButtonID )
    {
    case 0: // A on Xbox360 controller.
    case 8: // LS on Xbox360 controller.
        g_CameraMovement.TranslateFaster = false;
        break;
    case 9: // RS on XBox360 controller.
        g_CameraMovement.RotateFaster = false;
        break;
    case 5: // RB on XBox360 controller.
        g_CameraMovement.Up = 0.0f;
        break;
    case 4: // LB on XBox360 controller.
        g_CameraMovement.Down = 0.0f;
        break;
    }
}

void OnJoystickPOV( JoystickPOVEventArgs& e )
{
    switch ( e.Direction )
    {
    case JoystickPOVEventArgs::POVDirection::Centered:

        break;
    case JoystickPOVEventArgs::POVDirection::Up:

        break;
    case JoystickPOVEventArgs::POVDirection::UpRight:

        break;
    case JoystickPOVEventArgs::POVDirection::Right:
    {
        SelectNextLight();
    }
    break;
    case JoystickPOVEventArgs::POVDirection::DownRight:

        break;
    case JoystickPOVEventArgs::POVDirection::Down:

        break;
    case JoystickPOVEventArgs::POVDirection::DownLeft:

        break;
    case JoystickPOVEventArgs::POVDirection::Left:
    {
        SelectPreviousLight();
    }
    break;
    case JoystickPOVEventArgs::POVDirection::UpLeft:

        break;
    }
}

static float Threshold( float x, float threshold = 0.2f )
{
    return ( glm::abs( x ) < threshold ? 0.0f : x );
}

void OnJoystickAxis( JoystickAxisEventArgs& e )
{
    switch ( e.ChangedAxis )
    {
    case JoystickAxisEventArgs::JoystickAxis::XAxis:
        g_CameraMovement.Right = Threshold(e.Axis);
        break;
    case JoystickAxisEventArgs::JoystickAxis::YAxis:
        g_CameraMovement.Back = Threshold(e.Axis);
        break;
    case JoystickAxisEventArgs::JoystickAxis::ZAxis:
        g_CameraMovement.PivotTranslate = Threshold( e.Axis, 0.1f );
        break;
    case JoystickAxisEventArgs::JoystickAxis::RAxis:
        g_CameraMovement.Pitch = Threshold(e.Axis);
        break;
    case JoystickAxisEventArgs::JoystickAxis::UAxis:
        g_CameraMovement.Yaw = Threshold(-e.Axis);
        break;
    }
}

void OnFileChanged( FileChangeEventArgs& e )
{
    //std::wstringstream ss;

    //ss << "File modified: " << e.Path << std::endl;

    //OutputDebugStringW( ss.str().c_str() );
}

void TW_CALL SetRenderingTechnique( const void* value, void* clientdata )
{
    g_RenderingTechnique = *static_cast<const RenderingTechnique*>( value );
    g_RunningTime = 0.0;
    g_FrameCount = 0;
    g_FrameStatistic.Reset();
}

void TW_CALL GetRenderingTechnique( void* value, void* clientdata )
{
    *static_cast<RenderingTechnique*>( value ) = g_RenderingTechnique;
}

void TW_CALL SetNumLights( const void* value, void* clientdata )
{
    uint32_t numLights = *static_cast<const uint32_t*>( value );
    if ( numLights != g_Config.Lights.size() )
    {
        numLights = std::max<uint32_t>( numLights, 1 );

        g_Config.Lights.resize( numLights );

        g_uiCurrentLightIndex = std::min<uint32_t>( g_uiCurrentLightIndex, numLights - 1 );
        g_pCurrentLight = &g_Config.Lights[g_uiCurrentLightIndex];

        UpdateNumLights();
    }
}

void TW_CALL GetNumLights( void* value, void* clientdata )
{
    *static_cast<uint32_t*>( value ) = static_cast<uint32_t>( g_Config.Lights.size() );
}

void TW_CALL SetLightIndex( const void* value, void* clientdata )
{
    uint32_t lightIndex = *static_cast<const uint32_t*>( value );
    SetCurrentLight( std::min<uint32_t>( lightIndex, (uint32_t)(g_Config.Lights.size() - 1) ) );
}

void TW_CALL GetLightIndex( void* value, void* clientdata )
{
    *static_cast<uint32_t*>( value ) = g_uiCurrentLightIndex;
}

void TW_CALL SetLightType( const void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        g_pCurrentLight->m_Type = *static_cast<const Light::LightType*>( value );
    }
}

void TW_CALL GetLightType( void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        *static_cast<Light::LightType*>( value ) = g_pCurrentLight->m_Type;
    }
}

void TW_CALL SetLightPos( const void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        g_pCurrentLight->m_PositionWS = glm::vec4( *static_cast<const glm::vec3*>( value ), 1 );
    }
}

void TW_CALL GetLightPos( void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        *static_cast<glm::vec3*>( value ) = glm::vec3(g_pCurrentLight->m_PositionWS);
    }
}

void TW_CALL SetLightDir( const void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        g_pCurrentLight->m_DirectionWS = glm::vec4( *static_cast<const glm::vec3*>( value ), 0 );
    }
}

void TW_CALL GetLightDir( void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        *static_cast<glm::vec3*>( value ) = glm::vec3( g_pCurrentLight->m_DirectionWS );
    }
}

void TW_CALL SetLightColor( const void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        g_pCurrentLight->m_Color =  glm::vec4( *static_cast<const glm::vec3*>( value ), 1 );
    }
}

void TW_CALL GetLightColor( void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        *static_cast<glm::vec3*>( value ) = glm::vec3( g_pCurrentLight->m_Color );
    }
}

void TW_CALL SetSpotlightAngle( const void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        g_pCurrentLight->m_SpotlightAngle = *static_cast<const float*>( value );
    }
}

void TW_CALL GetSpotligtAngle( void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        *static_cast<float*>( value ) = g_pCurrentLight->m_SpotlightAngle;
    }
}

void TW_CALL SetLightRange( const void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        g_pCurrentLight->m_Range = *static_cast<const float*>( value );
    }
}

void TW_CALL GetLightRange( void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        *static_cast<float*>( value ) = g_pCurrentLight->m_Range;
    }
}

void TW_CALL SetLightIntensity( const void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        g_pCurrentLight->m_Intensity = *static_cast<const float*>( value );
    }
}

void TW_CALL GetLightIntensity( void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        *static_cast<float*>( value ) = g_pCurrentLight->m_Intensity;
    }
}

void TW_CALL SetLightEnabled( const void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        g_pCurrentLight->m_Enabled = *static_cast<const uint32_t*>( value );
    }
}

void TW_CALL GetLightEnabled( void* value, void* clientdata )
{
    if ( g_pCurrentLight && value )
    {
        *static_cast<bool*>( value ) = g_pCurrentLight->m_Enabled != 0;
    }
}

void TW_CALL GenerateLightsCB( void* clientdata )
{
    GenerateLights( g_Config.LightGenerationMethod, g_NumLightsToGenerate );
}

void TW_CALL GetAverageStatistic( void* value, void* clientData )
{
    Statistic* stat = static_cast<Statistic*>( clientData );
    *static_cast<double*>( value ) = stat->GetAverage();
}

void TW_CALL ResetStatisticsCB( void* clientdata )
{
    ResetStatistics();
}

void TW_CALL GetDeviceName( void* value, void* clientData )
{
    // Get: copy the value of g_DeviceName to AntTweakBar
    char** destPtr = static_cast<char**>( value );
    TwCopyCDStringToLibrary( destPtr, g_DeviceName.c_str() );
}

// Create Ant tweak bars for demo.
void CreateAntTweakBar()
{
    TwEnumVal twRenderingTechniqueEnum[] = {
        { int( RenderingTechnique::Forward ), "Forward" },
        { int( RenderingTechnique::Deferred ), "Deferred" },
        { int( RenderingTechnique::ForwardPlus ), "Forward Plus" }
    };
    TwType twRenderingEnumType = TwDefineEnum( "RenderingTechnique", twRenderingTechniqueEnum, _countof( twRenderingTechniqueEnum ) );

    TwEnumVal twGenerateMethodEnum[] = {
        { int(LightGeneration::Uniform), "Uniform" },
        { int(LightGeneration::Random), "Random" }
    };
    TwType twGenerateMethodEnumType = TwDefineEnum( "LightGeneration", twGenerateMethodEnum, _countof( twGenerateMethodEnum ) );

    TwEnumVal twLightTypeEnum[] = {
        { int( Light::LightType::Point ), "Point" },
        { int( Light::LightType::Spot ), "Spot" },
        { int( Light::LightType::Directional ), "Directional" }
    };
    TwType twLightEnumType = TwDefineEnum( "LightType", twLightTypeEnum, _countof( twLightTypeEnum ) );

    TwStructMember twVec3Struct[] = {
        { "X", TW_TYPE_FLOAT, offsetof( glm::vec3,x ), "step=0.01" },
        { "Y", TW_TYPE_FLOAT, offsetof( glm::vec3,y ), "step=0.01" },
        { "Z", TW_TYPE_FLOAT, offsetof( glm::vec3,z ), "step=0.01" },
    };
    TwType twVec3Type = TwDefineStruct( "vec3", twVec3Struct, _countof( twVec3Struct ), sizeof( glm::vec3 ), nullptr, nullptr );

    TwStructMember twVec4Struct[] = {
        { "X", TW_TYPE_FLOAT, offsetof( glm::vec4,x ), "step=0.01" },
        { "Y", TW_TYPE_FLOAT, offsetof( glm::vec4,y ), "step=0.01" },
        { "Z", TW_TYPE_FLOAT, offsetof( glm::vec4,z ), "step=0.01" },
        { "W", TW_TYPE_FLOAT, offsetof( glm::vec4,w ), "step=0.01" },
    };
    TwType twVec4Type = TwDefineStruct( "vec4", twVec4Struct, _countof( twVec4Struct ), sizeof( glm::vec4 ), nullptr, nullptr );

    // Rendering technique tweak bar.
    g_pRenderingTechniqueTweakBar = TwNewBar( "Rendering Technique" );
    TwDefine( "'Rendering Technique' refresh=0.5" );
    
    TwAddVarCB( g_pRenderingTechniqueTweakBar, "RenderingTechnique", twRenderingEnumType, &SetRenderingTechnique, &GetRenderingTechnique, nullptr, "label='Rendering Technique' help='Choose the rendering technique used to render the scene.'" );
    TwAddVarCB( g_pRenderingTechniqueTweakBar, "DeviceName", TW_TYPE_CDSTRING, nullptr, &GetDeviceName, nullptr, "label='Device Name' help='Primary graphics device name.'" );
    TwAddVarRO( g_pRenderingTechniqueTweakBar, "Window Width", TW_TYPE_UINT32, &g_WindowWidth, "label='Window Width' help='Window width in pixels.'" );
    TwAddVarRO( g_pRenderingTechniqueTweakBar, "Window Height", TW_TYPE_UINT32, &g_WindowHeight, "label='Window Height' help='Window height in pixels.'" );
    TwAddVarRW( g_pRenderingTechniqueTweakBar, "DebugLights", TW_TYPE_BOOLCPP, &g_RenderLights, "label='Render Debug Lights' help='Enable rendering of debug lights.' key=l" );
    TwAddVarRO( g_pRenderingTechniqueTweakBar, "Running Time", TW_TYPE_DOUBLE, &g_RunningTime, "label='Running Time' help='Running time in seconds.'" );
    TwAddVarRO( g_pRenderingTechniqueTweakBar, "Frame Count", TW_TYPE_UINT32, &g_FrameCount, "label='Frame Count'" );
    TwAddVarRO( g_pRenderingTechniqueTweakBar, "GPU Frame Time", TW_TYPE_DOUBLE, &g_FrameTime, "label='GPU Frame Time' help='GPU frame time in milliseconds.'" );
    TwAddVarCB( g_pRenderingTechniqueTweakBar, "GPU Frame Time (Avg)", TW_TYPE_DOUBLE, nullptr, &GetAverageStatistic, &g_FrameStatistic, "label='GPU Frame Time (Avg)' help='Average GPU frame time in milliseconds.'" );
    TwAddVarCB( g_pRenderingTechniqueTweakBar, "Forward Opaque Pass", TW_TYPE_DOUBLE, nullptr, &GetAverageStatistic, &g_ForwardOpaqueStatistic, "group='Forward Rendering' label='Opaque Pass'" );
    TwAddVarCB( g_pRenderingTechniqueTweakBar, "Forward Transparent Pass", TW_TYPE_DOUBLE, nullptr, &GetAverageStatistic, &g_ForwardTransparentStatistic, "group='Forward Rendering' label='Transparent Pass'" );
    TwAddVarCB( g_pRenderingTechniqueTweakBar, "Deferred G-Buffer Pass", TW_TYPE_DOUBLE, nullptr, &GetAverageStatistic, &g_DeferredGeometryStatistic, "group='Deferred Rendering' label='G-Buffer Pass'" );
    TwAddVarCB( g_pRenderingTechniqueTweakBar, "Deferred Lighting Pass", TW_TYPE_DOUBLE, nullptr, &GetAverageStatistic, &g_DeferredLightingStatistic, "group='Deferred Rendering' label='Lighting Pass'" );
    TwAddVarCB( g_pRenderingTechniqueTweakBar, "Deferred Transparent Pass", TW_TYPE_DOUBLE, nullptr, &GetAverageStatistic, &g_DeferredTransparentStatistic, "group='Deferred Rendering' label='Transparent Pass'" );
    TwAddVarCB( g_pRenderingTechniqueTweakBar, "Forward Plus Depth Prepass", TW_TYPE_DOUBLE, nullptr, &GetAverageStatistic, &g_ForwardPlusDepthPrepassStatistic, "group='Forward Plus' label='Depth Prepass'" );
    TwAddVarCB( g_pRenderingTechniqueTweakBar, "Forward Plus Light Culling", TW_TYPE_DOUBLE, nullptr, &GetAverageStatistic, &g_ForwardPlusLightCullingStatistic, "group='Forward Plus' label='Light Culling'" );
    TwAddVarCB( g_pRenderingTechniqueTweakBar, "Forward Plus Opaque Pass", TW_TYPE_DOUBLE, nullptr, &GetAverageStatistic, &g_ForwardPlusOpaqueStatistic, "group='Forward Plus' label='Opaque Pass'" );
    TwAddVarCB( g_pRenderingTechniqueTweakBar, "Forward Plus Transparent Pass", TW_TYPE_DOUBLE, nullptr, &GetAverageStatistic, &g_ForwardPlusTransparentStatistic, "group='Forward Plus' label='Transparent Pass'" );
    TwAddButton( g_pRenderingTechniqueTweakBar, "Reset Statistics", &ResetStatisticsCB, nullptr, "label='Reset Statistics' help='Reset statistics to 0'" );

    // Generate lights tweak bar.
    g_pGenerateLightsTweakBar = TwNewBar( "Generate Lights" );
    TwAddVarRW( g_pGenerateLightsTweakBar, "GenMethod", twGenerateMethodEnumType, &g_Config.LightGenerationMethod, "label='Light Generation Method' help='Determines How the lights are positioned in the scene.'" );
    TwAddVarRW( g_pGenerateLightsTweakBar, "LightCount", TW_TYPE_UINT32, &g_NumLightsToGenerate, "label='Light Count' min=1 max=16384 help='Number of lights to generate.'" );
    TwAddVarRW( g_pGenerateLightsTweakBar, "MinBounds", twVec3Type, &g_Config.LightsMinBounds, "label='Lights Min Bounds' help='Minimum Bounds for light generation.'" );
    TwAddVarRW( g_pGenerateLightsTweakBar, "MaxBounds", twVec3Type, &g_Config.LightsMaxBounds, "label='Lights Max Bounds' help='Maximum Bounds for light generation.'" );
    TwAddVarRW( g_pGenerateLightsTweakBar, "MinRange", TW_TYPE_FLOAT, &g_Config.MinRange, "label='Lights Min Range' min=0.01 step=0.01 help='Minumum light range.'" );
    TwAddVarRW( g_pGenerateLightsTweakBar, "MaxRange", TW_TYPE_FLOAT, &g_Config.MaxRange, "label='Lights Max Range' min=0.01 step=0.01 help='Maximum light range.'" );
    TwAddVarRW( g_pGenerateLightsTweakBar, "MinSpotAngle", TW_TYPE_FLOAT, &g_Config.MinSpotAngle, "label='Min Spot Angle' min=1 max=90 step=0.1 help='Minimum Spotlight Angle.'" );
    TwAddVarRW( g_pGenerateLightsTweakBar, "MaxSpotAngle", TW_TYPE_FLOAT, &g_Config.MaxSpotAngle, "label='Max Spot Angle' min=1 max=90 step=0.1 help='Maximum Spotlight Angle.'" );
    TwAddVarRW( g_pGenerateLightsTweakBar, "PointLights", TW_TYPE_BOOLCPP, &g_Config.GeneratePointLights, "label='Point Lights' help='Enable generation of point lights.'" );
    TwAddVarRW( g_pGenerateLightsTweakBar, "SpotLights", TW_TYPE_BOOLCPP, &g_Config.GenerateSpotLights, "label='Spot Lights' help='Enable generation of spot lights.'" );
    TwAddVarRW( g_pGenerateLightsTweakBar, "DirLights", TW_TYPE_BOOLCPP, &g_Config.GenerateDirectionalLights, "label='Dir Lights' help='Enable generation of directional lights.'" );
    TwAddButton( g_pGenerateLightsTweakBar, "GenerateLights", &GenerateLightsCB, nullptr, "label='Generate Lights' help='Generates lights in the scene.'" );

    // Light properties tweak bar.
    g_pLightsTweakBar = TwNewBar( "Selected Light" );
    TwAddVarCB( g_pLightsTweakBar, "NumLights", TW_TYPE_UINT32, nullptr, &GetNumLights, nullptr, "min=1 max=4096 keyincr=+ keydecr=- label='Num Lights'" );
    TwAddVarCB( g_pLightsTweakBar, "LightIndex", TW_TYPE_UINT32, &SetLightIndex, &GetLightIndex, nullptr, "min=0 max=0 label='Light Index'" ); // The max value will be determined by the size of the lights array.
    TwAddVarRW( g_pLightsTweakBar, "TrackCamera", TW_TYPE_BOOLCPP, &g_bLightTracksCamera, "label='Track Camera' help=`Light's position and direction will track the camera's pivot point.`" );

    TwAddVarCB( g_pLightsTweakBar, "LightType", twLightEnumType, &SetLightType, &GetLightType, nullptr, "group='Light Properties' label='Light Type'" );
    TwAddVarCB( g_pLightsTweakBar, "LightPos", twVec3Type, &SetLightPos, &GetLightPos, nullptr, "group='Light Properties' label='Light Position'" );
    TwAddVarCB( g_pLightsTweakBar, "LightDir", TW_TYPE_DIR3F, &SetLightDir, &GetLightDir, nullptr, "group='Light Properties' label='Light Direction'" );
    TwAddVarCB( g_pLightsTweakBar, "LightColor", TW_TYPE_COLOR3F, &SetLightColor, &GetLightColor, nullptr, "group='Light Properties' label='Light Color'" );
    TwAddVarCB( g_pLightsTweakBar, "SpotlightAngle", TW_TYPE_FLOAT, &SetSpotlightAngle, &GetSpotligtAngle, nullptr, "group='Light Properties' label='Spotlight Angle' min=1 max=90 step=0.1" );
    TwAddVarCB( g_pLightsTweakBar, "LightRange", TW_TYPE_FLOAT, &SetLightRange, &GetLightRange, nullptr, "group='Light Properties' label='Light Range' min=0.01 step=0.01" );
    TwAddVarCB( g_pLightsTweakBar, "LightIntensity", TW_TYPE_FLOAT, &SetLightIntensity, &GetLightIntensity, nullptr, "group='Light Properties' label='Light Intensity' min=0 max=10 step=0.01" );
    TwAddVarCB( g_pLightsTweakBar, "LightEnabled", TW_TYPE_BOOLCPP, &SetLightEnabled, &GetLightEnabled, nullptr, "group='Light Properties' label='Light Enabled'" );
}