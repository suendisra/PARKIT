/**
  @file     parkit.c
  @brief    Source file for PARKIT application
  @author   suendisra
*/
#include "parkit.h"

#define KEY_DELIM   L"\n"
#define KEY_MAX     32

static KEYBOARD kb = {0};

// static prototypes
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static void Defaults(void);
static BOOL File(const BOOL load);
static void Input(void);
static void Keys(const BOOL encrypt);
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
            switch(LOWORD(wp))
            {
                case TIMER_DISCOVERY_PULSE:
                    DiscoverMe();
                    break;
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
    MemClear(&app, sizeof(app));

    // assign random TCP port with simple increment to UDP port
    app.net.tcp = (USHORT)Random(NET_MIN_IPV_PORT, (NET_MAX_IPV_PORT - 1));
    app.net.udp = (app.net.tcp + 1);
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
                Keys(FALSE);
            }else{
                Keys(TRUE);
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

    // allow user to change mode (client<->server) if desired
    if(kb.up['M'] == TRUE)
    {
        // warn user that this shouldn't be done unless you're the network engineer
        /// TODO

        // wants to proceed, so do so
        /// TODO
    }

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

/* pull in all keys used to encrypt data on disk */
static void Keys(const BOOL encrypt)
{
    long    n = 0;
    wchar_t  keys[KEY_MAX * STR_NORM] = {0};
    wchar_t temp[STR_NORM] = {0};

    if(Resource(RSC_TYPE_TXT, IDR_PARKIT_KEYS, keys, sizeof(keys)) == TRUE)
    {
        // text file containing data-at-rest passwords has been opened
        for(n = 0; (n < KEY_MAX); ++n)
        {
            // use each key to encrypt or decrypt the app data as appropriate
            if(StrParse(temp, sizeof(temp), keys, sizeof(keys), KEY_DELIM, ((encrypt == TRUE) ? (n + 1) : (KEY_MAX - n))) == TRUE)
            {
                EncryptXOR(&app, sizeof(app), temp, StrLen(temp, sizeof(temp)));
            }
        }
    }
}

/* start application */
static BOOL Start(void)
{
    BOOL    success = FALSE;

    // load and set window icon
    if(Resource(RSC_TYPE_ICO, IDI_PARKIT, &wnd.icon, sizeof(wnd.icon)) == TRUE)
    {
        WindowIcon(wnd.handl, 0, wnd.icon);

        // attempt to load system-stored server configuration data
        if(File(TRUE) == FALSE)
        {
            // unable to load app data, set defaults
            Defaults();
        }

        // start up network operations
        if(Network(TRUE) == TRUE)
        {
            // start up GUI
            success = Interface(TRUE);
        }
    }

    return(success);
}

/* stop application */
static void Stop(void)
{
    // shut down network operations
    Network(FALSE);

    // shut down app
    Interface(FALSE);
    DestroyIcon(wnd.icon);
    Kill(wnd.handl);

    // save application settings
    File(FALSE);
}
