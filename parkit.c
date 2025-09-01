/**
  @file     parkit.c
  @brief    Source file for PARKIT application
  @author   suendisra
*/
#include "parkit.h"

#define FONT_NAME                   L"Consolas"
#define FONT_SIZE                   24

#define RATIO_SCREEN                0.96    // divisional value for main screen and info bar

static GPH      gph = {0};

static QUAD     area = {0};
static QUAD     info = {0};

static INDEX    font = -1;

static KEYBOARD kb = {0};

// static prototypes
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static void Defaults(void);
static void Draw(void);
static void DrawInfo(void);
static void DrawScreen(void);
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
            if(GphPaint(&gph, hwnd) == TRUE)
            {
                Draw();
                GphPaint(&gph, hwnd);
            }
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

/* draw primary application screen */
static void Draw(void)
{
    DrawInfo();
    DrawScreen();
}

/* draw informational text regarding application states */
static void DrawInfo(void)
{
    // clear the background and info bar
    GphClear(gph, &info, GCOAL);

    /// TODO

    // blit and display
    GphBlit(gph);
}

/* draw main application screen with parking lot information */
static void DrawScreen(void)
{
    // clear the background and info bar
    GphClear(gph, &area, GBLACK);

    /// TODO

    // blit and display
    GphBlit(gph);
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

            // divide screen into main drawing area and informational bar
            QuadDivideH(wnd.client, RATIO_SCREEN, &area, &info);

            // create the Windows GDI object
            if(Gph(wnd.handl, wnd.client, &gph) == TRUE)
            {
                // create primary font for display
                font = FontGDI(FONT_NAME, FONT_SIZE, FALSE, FALSE, FALSE);
                if(font >= 0)
                {
                    // set the font text and color
                    GphFont(&gph, font);
                    GphFontColor(gph, GGRAY, GTRANS);

                    /// TODO - remove this later
                    app.server = TRUE;

                    // start up the server/client
                    success = ((app.server == TRUE) ? Server(TRUE) : Client(TRUE));
                }
            }
        }
    }

    return(success);
}

/* stop application */
static void Stop(void)
{
    // unload all allocated memory from demos
    FontKillGDI(font);

    // stop GDI side of application
    GphKill(&gph);

    // shut down app
    DestroyIcon(wnd.icon);
    Kill(wnd.handl);

    // shut down network operations
    if(app.server == TRUE)
    {
        Server(FALSE);
    }else{
        Client(FALSE);
    }
    Net(FALSE);
}
