#pragma once

#include "RenderWindow.h"

class Application;

class ProgressWindow : public RenderWindow
{
public:
    typedef RenderWindow base;

    virtual void ShowWindow();
    virtual void HideWindow();
    virtual void CloseWindow();

    virtual void Present();
    virtual std::shared_ptr<RenderTarget> GetRenderTarget();

    void SetTotalProgress( float totalProgress );
    void UpdateProgress( ProgressEventArgs& e );

protected:
    friend Application;
    ProgressWindow( Application& app, HWND hWnd, int width, int height, float totalProgress = 100.0f );
    virtual ~ProgressWindow();

private:
    HWND m_hParentWindow;
    HWND m_hProgressWindow;
    float m_fTotalProgress;
    float m_fCurrentProgress;
    // Keep track of the previous progress 
    // value sent so we can compute a delta..
    float m_fPreviousProgress;
};