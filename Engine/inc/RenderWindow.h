#pragma once

#include <Object.h>
#include <Events.h>

// For JOYINFOEX structure
#include <joystickapi.h>

#define NUM_SUPPORTED_JOYSTICKS 16

class Camera;
class Buffer;
class StructuredBuffer;
class Scene;
class Mesh;
class Shader;
class Texture;
class SamplerState;
class Rect;
class Material;
class RenderTarget;

class RenderWindow : public Object
{
public:
    typedef Object base;

    // Show this window if it is hidden.
    virtual void ShowWindow() = 0;
    // Hide the window. The window will not be destroyed and can be 
    // shown again using the ShowWindow() function.
    virtual void HideWindow() = 0;

    // Destroy and close the window.
    virtual void CloseWindow() = 0;

    const std::string& GetWindowName() const;

    int GetWindowWidth() const;
    int GetWindowHeight() const;

    bool IsVSync() const;

    // Present the back buffers
    virtual void Present() = 0;

    // Get the render target of this render window.
    virtual std::shared_ptr<RenderTarget> GetRenderTarget() = 0;

    // Invoked when the window is initialized.
    Event				Initialize;
    Event				Terminate;

    // Window is closing
    WindowCloseEvent    Close;

    // Window events
    // Window gets input focus
    Event           InputFocus;
    // Window loses input focus
    Event           InputBlur;
    // Window is minimized.
    Event           Minimize;
    // Window is restored.
    Event           Restore;

    // Invoked when the window contents should be repainted.
    // (Part of the window is now "exposed".
    Event           Expose;

    // Update event is called when the application
    // will be updated before rendering.
    // This this callback to update your game logic.
    UpdateEvent			Update;

    // Invoked when the window needs to be redrawn.
    // Pre-render events are useful for things like updating 
    // effect parameters.
    RenderEvent         PreRender;
    RenderEvent         Render;
    // Post-render events are useful for things like drawing the GUI
    // interface or performing post-process effects.
    RenderEvent         PostRender;

    // Keyboard events
    KeyboardEvent       KeyPressed;
    KeyboardEvent       KeyReleased;
    // Window gained keyboard focus
    Event               KeyboardFocus;
    // Window lost keyboard focus.
    Event               KeyboardBlur;

    // The mouse was moved over the window
    MouseMotionEvent    MouseMoved;
    // A mouse button was pressed over the window
    MouseButtonEvent    MouseButtonPressed;
    // A mouse button was released over the window.
    MouseButtonEvent    MouseButtonReleased;
    // The mouse wheel was changed
    MouseWheelEvent     MouseWheel;
    // The mouse left the client area.
    Event               MouseLeave;
    // The window has gained the mouse focus.
    Event               MouseFocus;
    // The window has lost mouse focus.
    Event               MouseBlur;

    // Joystick events
    JoystickButtonEvent JoystickButtonPressed;
    JoystickButtonEvent JoystickButtonReleased;
    // Event is fired when the POV on the joystick changes.
    JoystickPOVEvent JoystickPOV;
    // Event is fired if any of the axes of the joystick change.
    // No thresholding is applied.
    JoystickAxisEvent JoystickAxis;

    // The window was resized.
    ResizeEvent         Resize;

protected:
    friend LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
    friend class Application;

    // Only the application can create windows.
    RenderWindow( Application& theApp, const std::string& windowName, int windowWidth, int windowHeight, bool vSync = false );
    virtual ~RenderWindow();

    // Get the application that was used to create the window.
    Application& GetApplication();

    virtual void OnInitialize( EventArgs& e );

    // The application window has received focus
    virtual void OnInputFocus( EventArgs& e );
    // The application window has lost focus
    virtual void OnInputBlur( EventArgs& e );


    // The application window has been minimized
    virtual void OnMinimize( EventArgs& e );
    // The application window has been restored
    virtual void OnRestore( EventArgs& e );


    // The application window has be resized
    virtual void OnResize( ResizeEventArgs& e );
    // The window contents should be repainted
    virtual void OnExpose( EventArgs& e );

    // The user requested to exit the application
    virtual void OnClose( WindowCloseEventArgs& e );

    virtual void OnUpdate( UpdateEventArgs& e );
    virtual void OnPreRender( RenderEventArgs& e );
    virtual void OnRender( RenderEventArgs& e );
    virtual void OnPostRender( RenderEventArgs& e );

    // A keyboard key was pressed
    virtual void OnKeyPressed( KeyEventArgs& e );
    // A keyboard key was released
    virtual void OnKeyReleased( KeyEventArgs& e );
    // Window gained keyboard focus
    virtual void OnKeyboardFocus( EventArgs& e );
    // Window lost keyboard focus
    virtual void OnKeyboardBlur( EventArgs& e );

    // The mouse was moved
    virtual void OnMouseMoved( MouseMotionEventArgs& e );
    // A button on the mouse was pressed
    virtual void OnMouseButtonPressed( MouseButtonEventArgs& e );
    // A button on the mouse was released
    virtual void OnMouseButtonReleased( MouseButtonEventArgs& e );
    // The mouse wheel was moved.
    virtual void OnMouseWheel( MouseWheelEventArgs& e );
    // The mouse left the client are of the window.
    virtual void OnMouseLeave( EventArgs& e );
    // The application window has received mouse focus
    virtual void OnMouseFocus( EventArgs& e );
    // The application window has lost mouse focus
    virtual void OnMouseBlur( EventArgs& e );

    virtual void OnJoystickButtonPressed( JoystickButtonEventArgs& e );
    virtual void OnJoystickButtonReleased( JoystickButtonEventArgs& e );
    virtual void OnJoystickPOV( JoystickPOVEventArgs& e );
    virtual void OnJoystickAxis( JoystickAxisEventArgs& e );

    // The application is terminating.
    virtual void OnTerminate( EventArgs& e );

private:
    // Process joystick input and fire joystick events.
    void ProcessJoysticks();

    Application& m_Application;

    int m_iWindowWidth;
    int m_iWindowHeight;

    bool m_vSync;

    std::string m_sWindowName;

    // Used to compute relative mouse motion in this window.
    glm::ivec2 m_PreviousMousePosition;

    // The state of the joystick buttons the last time we polled the joystick events.
    JOYINFOEX m_PreviousJoystickInfo[NUM_SUPPORTED_JOYSTICKS];

    // This is true when the mouse is inside the window's client rect.
    bool m_bInClientRect;

    // This is set to true when the window receives keyboard focus.
    bool m_bHasKeyboardFocus;

    // Keep track of the event connections so that I can disconnect them if the
    // window is closed.
    // Also automatically closes any registered events when the window is destroyed.
    Event::ScopedConnections m_EventConnections;

};
