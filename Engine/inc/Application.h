#pragma once
#include "Events.h"
#include "Object.h"

#include "ReadDirectoryChanges.h"

class RenderDevice;
class RenderWindow;
class ProgressWindow;

class Application : public Object
{
public:
    typedef Object base;

    Application();
    virtual ~Application();

    // Get the singleton instance to the application.
    static Application& Get();

    // Run the application
    // Will return the exit code.
    virtual int Run();
    // Stop the app from running and close the application window.
    virtual void Stop();

    // Create a new render Window with the specified dimensions.
    RenderWindow& CreateRenderWindow( const std::string& title, int windowWidth, int windowHeight, bool vSync = false );
    ProgressWindow& CreateProgressWindow( const std::string& title, int windowWidth, int windowHeight );
    // Get the render device associated with this application.
    RenderDevice& GetRenderDevice();

    // Get the module handle for this instance of the application.
    // TODO: This should be abstracted away into a specific Application class.
    HINSTANCE GetModuleHandle() const;

    // Retrieve the total time the application has been running.
    float GetTotalApplicationTime() const;

    // Register a directory for receiving file change notifications.
    void RegisterDirectoryChangeListener( const std::wstring& dir, bool recursive = true );

    // Application execution events
    Event           Initialize;
    UpdateEvent     Update;

    // Called when the application is exiting.
    Event           Exit;

    // This event is called when the application is going to be terminated.
    Event			Terminate;
    // This event is called after the application has been terminated.
    Event           Terminated;

    // A file has been modified on disk.
    FileChangeEvent FileChanged;

    // User events
    UserEvent        UserEvent;

protected:

    // Invoked when the application is initialized.
    // This is called when the application is run.
    // This allows windows that have been created to receive this event.
    virtual void OnInitialize( EventArgs& e );

    virtual void OnUpdate( UpdateEventArgs& e );
    virtual void OnRender( RenderEventArgs& e );

    // The application instance is going to be terminated.
    virtual void OnTerminate( EventArgs& e );
    // The application instance has been terminated.
    virtual void OnTerminated( EventArgs& e );

    // The user requested to exit the application
    virtual void OnExit( EventArgs& e );

    // A user event was generated.
    virtual void OnUserEvent( UserEventArgs& e );

    // A file modification was detected.
    virtual void OnFileChange( FileChangeEventArgs& e );

    // Check for any file changes that may have occurred.
    void CheckFileChanges();

private:
    // Accumulated timer for application time.
    float           m_fTotalTime;

    bool            m_bIsInitialized;
    bool            m_bIsRunning;

    // Directory change listener.
    CReadDirectoryChanges m_DirectoryChanges;
    // Thread to run directory change listener.
    std::thread m_DirectoryChangeListenerThread;
    // Mutex to protect changes to directory change listener.
    typedef std::recursive_mutex Mutex;
    typedef std::unique_lock<Mutex> MutexLock;
    Mutex m_DirectoryChangesMutex;
    // Flag to terminate directory change thread.
    std::atomic<bool> m_bTerminateDirectoryChangeThread;

    // Handle to the module.
    HINSTANCE m_hInstance;

    RenderDevice*   m_pRenderDevice;

    typedef std::map<std::string, RenderWindow*> WindowMap;
    WindowMap m_Windows;
};