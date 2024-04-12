#include <EnginePCH.h>

#include <Application.h>

#include <RenderWindow.h>
#include <Camera.h>

#include "../resource.h"

RenderWindow::RenderWindow( Application& app, const std::string& windowName, int windowWidth, int windowHeight, bool vSync )
    : m_Application( app )
    , m_sWindowName( windowName )
    , m_iWindowWidth( windowWidth )
    , m_iWindowHeight( windowHeight )
    , m_vSync( vSync )
    , m_PreviousMousePosition( 0, 0 )
    , m_bInClientRect( false )
    , m_bHasKeyboardFocus( false )
{
    // Clear previous joystick info array
    memset( m_PreviousJoystickInfo, 0, sizeof( m_PreviousJoystickInfo ) );
    // Clear previous joystick pov state.
    for ( int i = 0; i < NUM_SUPPORTED_JOYSTICKS; i++ )
    {
        m_PreviousJoystickInfo[i].dwPOV = JOY_POVCENTERED;
        m_PreviousJoystickInfo[i].dwRpos = 0x7fff;
//        m_PreviousJoystickInfo[i].dwUpos = 0x7fff;
//        m_PreviousJoystickInfo[i].dwVpos = 0x7fff;
        m_PreviousJoystickInfo[i].dwXpos = 0x7fff;
        m_PreviousJoystickInfo[i].dwYpos = 0x7fff;
        m_PreviousJoystickInfo[i].dwZpos = 0x7fff;
    }

    // Register initialize/terminate events.
    m_EventConnections.push_back( app.Initialize += boost::bind( &RenderWindow::OnInitialize, this, _1 ) );
    m_EventConnections.push_back( app.Terminate += boost::bind( &RenderWindow::OnTerminate, this, _1 ) );

    // Register update/render events.
    m_EventConnections.push_back( app.Update += boost::bind( &RenderWindow::OnUpdate, this, _1 ) );
}

RenderWindow::~RenderWindow()
{
    // Disconnect events.
    for ( Event::ConnectionType& eventConnection : m_EventConnections )
    {
        eventConnection.disconnect();
    }
}

void RenderWindow::ShowWindow()
{}

void RenderWindow::HideWindow()
{}

void RenderWindow::CloseWindow()
{
    // Disconnect events.
    for ( Event::ConnectionType& eventConnection : m_EventConnections )
    {
        eventConnection.disconnect();
    }
    m_EventConnections.clear();
}

Application& RenderWindow::GetApplication()
{
    return m_Application;
}

const std::string& RenderWindow::GetWindowName() const
{
    return m_sWindowName;
}

int RenderWindow::GetWindowWidth() const
{
    return m_iWindowWidth;
}

int RenderWindow::GetWindowHeight() const
{
    return m_iWindowHeight;
}

bool RenderWindow::IsVSync() const
{
    return m_vSync;
}

void RenderWindow::OnInitialize( EventArgs& e )
{
    Initialize( e );
}

void RenderWindow::OnTerminate( EventArgs& e )
{
    Terminate( e );
}
// The window has received focus
void RenderWindow::OnInputFocus( EventArgs& e )
{
    InputFocus( e );
}

// The RenderWindow window has lost focus
void RenderWindow::OnInputBlur( EventArgs& e )
{
    InputBlur( e );
}

// The RenderWindow window has been minimized
void RenderWindow::OnMinimize( EventArgs& e )
{
    Minimize( e );
}

// The RenderWindow window has been restored
void RenderWindow::OnRestore( EventArgs& e )
{
    Restore( e );
}

// The RenderWindow window has be resized
void RenderWindow::OnResize( ResizeEventArgs& e )
{
    m_iWindowWidth = e.Width;
    m_iWindowHeight = e.Height;

    Resize( e );
}

// The window contents should be repainted
void RenderWindow::OnExpose( EventArgs& e )
{
    Expose( e );
}

// Normalize a value in the range [min - max]
template<typename T, typename U>
inline T normalizeRange( U x, U min, U max )
{
    return T( x - min ) / T( max - min );
}

// Shift and bias a value into another range.
template<typename T, typename U>
inline T shiftBias( U x, U shift, U bias )
{
    return T( x * bias ) + T( shift );
}

void RenderWindow::ProcessJoysticks()
{
    // Process joystick input 
    // and fire appropriate events.
    JOYINFOEX joystickInfo;
    joystickInfo.dwSize = sizeof( JOYINFOEX );
    joystickInfo.dwFlags = JOY_RETURNALL | JOY_RETURNPOVCTS;

    JOYCAPS joystickCapabilities;


    for ( UINT joystickID = 0; joystickID < NUM_SUPPORTED_JOYSTICKS; joystickID++ )
    {
        // Skip joysticks that generate errors.
        if ( joyGetPosEx( joystickID, &joystickInfo ) != JOYERR_NOERROR ) continue;
        joyGetDevCaps( joystickID, &joystickCapabilities, sizeof( JOYCAPS ) );

        DWORD currentButtons = joystickInfo.dwButtons;
        DWORD previousButtons = m_PreviousJoystickInfo[joystickID].dwButtons;
        bool buttonStates[32];
        int i;
        DWORD buttonMask;

        for ( i = 0, buttonMask = 1; i < 32; i++, buttonMask = buttonMask << 1 )
        {
            buttonStates[i] = ( currentButtons & buttonMask ) != 0;
        }

        // Which buttons are now pressed this frame.
        DWORD buttonPressed = currentButtons & ~previousButtons;
        // Which buttons are still being held since the last frame.
        DWORD buttonHeld = currentButtons & previousButtons;
        // Which buttons were released this frame.
        DWORD buttonReleased = previousButtons & ~currentButtons;

        // Fire an event for every button that is now "pressed"
        for ( i = 0, buttonMask = 1; i < 32; i++, buttonMask = buttonMask << 1 )
        {
            if ( ( buttonPressed & buttonMask ) != 0 )
            {
                JoystickButtonEventArgs joyEventArgs( *this, joystickID, JoystickButtonEventArgs::Pressed, i, buttonStates );
                OnJoystickButtonPressed( joyEventArgs );
            }
        }
        // Fire an event for every button that is now "released"
        for ( i = 0, buttonMask = 1; i < 32; i++, buttonMask = buttonMask << 1 )
        {
            if ( ( buttonReleased & buttonMask ) != 0 )
            {
                JoystickButtonEventArgs joyEventArgs( *this, joystickID, JoystickButtonEventArgs::Released, i, buttonStates );
                OnJoystickButtonReleased( joyEventArgs );
            }
        }

        // Check Point of View hat
        // Both continuous and discrete values are handled similarly.
        if ( m_PreviousJoystickInfo[joystickID].dwPOV != joystickInfo.dwPOV )
        {
            float angle = joystickInfo.dwPOV / 100.0f;
            // Compute the discreet interval.
            JoystickPOVEventArgs::POVDirection dir = JoystickPOVEventArgs::POVDirection::Centered;
            if ( angle >= 0.0f && angle < 360.0f )
            {
                // Convert the continuous angle value into a discreet interval.
                dir = ( JoystickPOVEventArgs::POVDirection )( (int)( glm::round<int>( (int)( ( angle / 360.0f ) * 8.0f ) * 45 ) ) );
            }
            else
            {
                angle = -1.0f;
            }

            JoystickPOVEventArgs joyEventArgs( *this, joystickID, angle, dir, buttonStates );
            OnJoystickPOV( joyEventArgs );
        }

        if ( m_PreviousJoystickInfo[joystickID].dwRpos != joystickInfo.dwRpos )
        {
            float axis = shiftBias<float>( normalizeRange<float, UINT>( joystickInfo.dwRpos, joystickCapabilities.wRmin, joystickCapabilities.wRmax ), -1.0f, 2.0f );
            JoystickAxisEventArgs joyEventArgs( *this, joystickID, JoystickAxisEventArgs::JoystickAxis::RAxis, axis, buttonStates );
            OnJoystickAxis( joyEventArgs );
        }
        if ( m_PreviousJoystickInfo[joystickID].dwUpos != joystickInfo.dwUpos )
        {
            float axis = shiftBias<float>( normalizeRange<float, UINT>( joystickInfo.dwUpos, joystickCapabilities.wUmin, joystickCapabilities.wUmax ), -1.0f, 2.0f );
            JoystickAxisEventArgs joyEventArgs( *this, joystickID, JoystickAxisEventArgs::JoystickAxis::UAxis, axis, buttonStates );
            OnJoystickAxis( joyEventArgs );
        }
        if ( m_PreviousJoystickInfo[joystickID].dwVpos != joystickInfo.dwVpos )
        {
            float axis = shiftBias<float>( normalizeRange<float, UINT>( joystickInfo.dwVpos, joystickCapabilities.wVmin, joystickCapabilities.wVmax ), -1.0f, 2.0f );
            JoystickAxisEventArgs joyEventArgs( *this, joystickID, JoystickAxisEventArgs::JoystickAxis::VAxis, axis, buttonStates );
            OnJoystickAxis( joyEventArgs );
        }
        if ( m_PreviousJoystickInfo[joystickID].dwXpos != joystickInfo.dwXpos )
        {
            float axis = shiftBias<float>( normalizeRange<float, UINT>( joystickInfo.dwXpos, joystickCapabilities.wXmin, joystickCapabilities.wXmax ), -1.0f, 2.0f );
            JoystickAxisEventArgs joyEventArgs( *this, joystickID, JoystickAxisEventArgs::JoystickAxis::XAxis, axis, buttonStates );
            OnJoystickAxis( joyEventArgs );
        }
        if ( m_PreviousJoystickInfo[joystickID].dwYpos != joystickInfo.dwYpos )
        {
            float axis = shiftBias<float>( normalizeRange<float, UINT>( joystickInfo.dwYpos, joystickCapabilities.wYmin, joystickCapabilities.wYmax ), -1.0f, 2.0f );
            JoystickAxisEventArgs joyEventArgs( *this, joystickID, JoystickAxisEventArgs::JoystickAxis::YAxis, axis, buttonStates );
            OnJoystickAxis( joyEventArgs );
        }
        if ( m_PreviousJoystickInfo[joystickID].dwZpos != joystickInfo.dwZpos )
        {
            float axis = shiftBias<float>( normalizeRange<float, UINT>( joystickInfo.dwZpos, joystickCapabilities.wZmin, joystickCapabilities.wZmax ), -1.0f, 2.0f );
            JoystickAxisEventArgs joyEventArgs( *this, joystickID, JoystickAxisEventArgs::JoystickAxis::ZAxis, axis, buttonStates );
            OnJoystickAxis( joyEventArgs );
        }

        // Store the current joystick info for the next frame.
        m_PreviousJoystickInfo[joystickID] = joystickInfo;
    }

}


void RenderWindow::OnUpdate( UpdateEventArgs& e )
{
    // Don't processs joystick events unless the window has keyboard focus
    // (The window is selected and can receive keyboard events)
//    if ( m_bHasKeyboardFocus )
    {
        ProcessJoysticks();
    }

    Update( e );
}

// The prepare the window for redraw (update shader parameters)
void RenderWindow::OnPreRender( RenderEventArgs& e )
{
    RenderEventArgs renderArgs( *this, e.ElapsedTime, e.TotalTime, e.FrameCounter, e.Camera, e.PipelineState );
    PreRender( renderArgs );
}

// The window should be redrawn.
void RenderWindow::OnRender( RenderEventArgs& e )
{
    RenderEventArgs renderArgs( *this, e.ElapsedTime, e.TotalTime, e.FrameCounter, e.Camera, e.PipelineState );
    Render( renderArgs );
}

// Handle any post-drawing events (like GUI)
void RenderWindow::OnPostRender( RenderEventArgs& e )
{
    RenderEventArgs renderArgs( *this, e.ElapsedTime, e.TotalTime, e.FrameCounter, e.Camera, e.PipelineState );
    PostRender( renderArgs );
}

// A keyboard key was pressed
void RenderWindow::OnKeyPressed( KeyEventArgs& e )
{
    KeyPressed( e );
}

// A keyboard key was released
void RenderWindow::OnKeyReleased( KeyEventArgs& e )
{
    KeyReleased( e );
}

// Window gained keyboard focus
void RenderWindow::OnKeyboardFocus( EventArgs& e )
{
    m_bHasKeyboardFocus = true;
    KeyboardFocus( e );
}

// Window lost keyboard focus
void RenderWindow::OnKeyboardBlur( EventArgs& e )
{
    m_bHasKeyboardFocus = false;
    KeyboardBlur( e );
}

// The mouse was moved
void RenderWindow::OnMouseMoved( MouseMotionEventArgs& e )
{
    if ( !m_bInClientRect )
    {
        m_PreviousMousePosition = glm::ivec2( e.X, e.Y );
        m_bInClientRect = true;
    }

    e.RelX = e.X - m_PreviousMousePosition.x;
    e.RelY = e.Y - m_PreviousMousePosition.y;

    m_PreviousMousePosition = glm::ivec2( e.X, e.Y );

    MouseMoved( e );
}

// A button on the mouse was pressed
void RenderWindow::OnMouseButtonPressed( MouseButtonEventArgs& e )
{
    MouseButtonPressed( e );
}

// A button on the mouse was released
void RenderWindow::OnMouseButtonReleased( MouseButtonEventArgs& e )
{
    MouseButtonReleased( e );
}

void RenderWindow::OnMouseWheel( MouseWheelEventArgs& e )
{
    MouseWheel( e );
}

void RenderWindow::OnMouseLeave( EventArgs& e )
{
    m_bInClientRect = false;
    MouseLeave( e );
}

// The window has received mouse focus
void RenderWindow::OnMouseFocus( EventArgs& e )
{
    MouseFocus( e );
}

// The window has lost mouse focus
void RenderWindow::OnMouseBlur( EventArgs& e )
{
    MouseBlur( e );
}

void RenderWindow::OnJoystickButtonPressed( JoystickButtonEventArgs& e )
{
    JoystickButtonPressed( e );
}

void RenderWindow::OnJoystickButtonReleased( JoystickButtonEventArgs& e )
{
    JoystickButtonReleased( e );
}

void RenderWindow::OnJoystickPOV( JoystickPOVEventArgs& e )
{
    JoystickPOV( e );
}

void RenderWindow::OnJoystickAxis( JoystickAxisEventArgs& e )
{
    JoystickAxis( e );
}

void RenderWindow::OnClose( WindowCloseEventArgs& e )
{
    Close( e );
}
