/**
  @file     parkit.c
  @brief    Source file for PARKIT application
  @author   suendisra
*/
#include "parkit.h"

static KEYBOARD kb = {0};

// static prototypes
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static void Defaults(void);
static BOOL File(const BOOL load);
static void Input(void);
static BOOL Start(void);
static void Stop(void);

/* wWinMain */
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previnst, LPWSTR cmd, int show)
{
    WinInit(instance, previnst, cmd, show);

    // create window and loop it
    Dims(NULL, 0, NULL, &wnd.client);
    if(Window(MainWndProc, APP_TITLE, wnd.client, NULL, &wnd) == TRUE)
    {
        // set up looping parameters
        wnd.mode = LOOP_GET;
        wnd.init = Start;
        wnd.stop = Stop;

        // loop application window procedure
        Loop(&wnd);
    }

    return(0);
}

/* MainWndProc */
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    long    rval = 0;

    switch(msg)
    {
        case WM_CREATE:
            TextSet(hwnd, 0, L"%s - %s", APP_TITLE, APP_VERSION);
            break;

        case WM_ACTIVATE:
            Focus(hwnd, 0);
            break;

        case WM_PAINT:
            Draw(hwnd);
            break;

        case WM_SYSCOMMAND:
            switch(LOWORD(wp))
            {
                case SC_CLOSE:
                case SC_DEFAULT:
                    Kill(hwnd);
                    break;

                default:
                    rval = DefWindowProc(hwnd, msg, wp, lp);
                    break;
            }
            break;

        case WM_TIMER:
            if(app.server == TRUE)
            {
                Heartbeat();
            }
            break;

        case WM_KEYDOWN:
        case WM_KEYUP:
            if(KeySet(wp, (msg == WM_KEYDOWN), &kb) == TRUE)
            {
                Input();

                // up-state of key(s) should not be persistent
                MemClear(&kb.up, sizeof(kb.up));
            }
            break;

        default:
            rval = DefWindowProc(hwnd, msg, wp, lp);
            break;
    }

    return(rval);
}

/* set up standard client-side defaults */
static void Defaults(void)
{
    app.server = FALSE;
}

/* attempt to read in or write out system-stored server configuration */
static BOOL File(const BOOL load)
{
    CFILE   cf = {0};
    wchar_t path[STR_PATH] = {0};
    BOOL    success = FALSE;

    // get system path for save data
    if(FolderApp(path, sizeof(path), L"%s", APP_PATH) == TRUE)
    {
        // open save data
        if(FileOpen(((load == TRUE) ? FILE_READ : FILE_WRITE), path, &cf) == TRUE)
        {
            // iterate through each file_save or file_load call for demos
            if(load == TRUE)
            {
                success = (FileRead(cf, &app, sizeof(app)) == sizeof(app));
            }else{
                success = (FileWrite(cf, &app, sizeof(app)) == sizeof(app));
            }

            FileClose(&cf);
        }
    }

    return(success);
}

/* handle user input */
static void Input(void)
{
    BOOL    input = FALSE;

    // allow the user to exit the application just from Esc or 'X'
    if((kb.up['X'] == TRUE) || (kb.up[VK_ESCAPE] == TRUE))
    {
        SendMessage(wnd.handl, WM_SYSCOMMAND, SC_CLOSE, 0);
    }

    // redraw screen after user input
    if(input == TRUE)
    {
        SendMessage(wnd.handl, WM_PAINT, 0, 0);
    }
}

/* start application */
static BOOL Start(void)
{
    BOOL    success = FALSE;

    // start up network operations
    if(Net(TRUE) == TRUE)
    {
        // attempt to load system-stored server configuration data
        if(File(TRUE) == TRUE)
        {
            // unable to load app data, set defaults
            Defaults();
        }

        // load and set window icon
        if(Resource(RSC_TYPE_ICO, IDI_PARKIT, &wnd.icon, sizeof(wnd.icon)) == TRUE)
        {
            WindowIcon(wnd.handl, 0, wnd.icon);

            if(Interface(TRUE) == TRUE)
            {
                /// TODO - remove this later
                app.server = TRUE;

                // start up the server/client
                success = ((app.server == TRUE) ? Server(TRUE) : Client(TRUE));
            }
        }
    }

    return(success);
}

/* stop application */
static void Stop(void)
{
    // shut down network operations
    if(app.server == TRUE)
    {
        Server(FALSE);
    }else{
        Client(FALSE);
    }
    Net(FALSE);

    // shut down app
    Interface(FALSE);
    DestroyIcon(wnd.icon);
    Kill(wnd.handl);

    // save application settings
    File(FALSE);
}
