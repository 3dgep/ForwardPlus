#include <EnginePCH.h>

#include <Application.h>
#include <ProgressWindow.h>

ProgressWindow::ProgressWindow( Application& app, HWND hWnd, int width, int height, float totalProgress )
    : base( app, "", width, height )
    , m_hParentWindow( hWnd )
    , m_fTotalProgress( totalProgress )
    , m_fCurrentProgress( 0.0f )
    , m_fPreviousProgress( 0.0f )
{
    RECT clientArea;

    GetClientRect( m_hParentWindow, &clientArea );

    m_hProgressWindow = CreateWindowEx( 0, PROGRESS_CLASS, nullptr, WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
                                        clientArea.left, clientArea.top, clientArea.right - clientArea.left, clientArea.bottom - clientArea.top,
                                        m_hParentWindow, 0, app.GetModuleHandle(), nullptr );

    if ( !m_hProgressWindow )
    {
        ReportError( "Failed to create progress window." );
        return;
    }

    SetTotalProgress( totalProgress );
}

ProgressWindow::~ProgressWindow()
{
    CloseWindow();
}

void ProgressWindow::ShowWindow()
{
    base::ShowWindow();
    ::ShowWindow( m_hParentWindow, SW_SHOW );
}
void ProgressWindow::HideWindow()
{
    base::HideWindow();
    ::ShowWindow( m_hParentWindow, SW_HIDE );
}
void ProgressWindow::CloseWindow()
{
    base::CloseWindow();

    if ( m_hProgressWindow )
    {
        ::DestroyWindow( m_hProgressWindow );
        m_hProgressWindow = 0;
    }
    if ( m_hParentWindow )
    {
        ::DestroyWindow( m_hParentWindow );
        m_hProgressWindow = 0;
    }
}

void ProgressWindow::Present()
{

}

std::shared_ptr<RenderTarget> ProgressWindow::GetRenderTarget()
{
    return std::shared_ptr<RenderTarget>();
}


void ProgressWindow::SetTotalProgress( float totalProgress )
{
    m_fTotalProgress = totalProgress;
    ::SendMessage( m_hProgressWindow, PBM_SETRANGE, 0, MAKELPARAM( 0, ( totalProgress * 100.0f ) ) ); // Add 100 because we're dealing with integer values.
}

void ProgressWindow::UpdateProgress( ProgressEventArgs& e )
{
    if ( e.Progress < m_fPreviousProgress )
    {
        m_fCurrentProgress += ( 1.0f - m_fPreviousProgress );
        m_fPreviousProgress = e.Progress;
    }
    else
    {
        m_fCurrentProgress += ( e.Progress - m_fPreviousProgress );
    }

    m_fPreviousProgress = e.Progress;

    ::SendMessage( m_hProgressWindow, PBM_SETPOS, (WPARAM)( m_fCurrentProgress * 100.0f ), 0 );
}

