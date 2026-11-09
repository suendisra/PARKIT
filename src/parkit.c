/**
  @file     parkit.c
  @brief    Source file for PARKIT application
*/
#include "parkit.h"

#include <gph-std.h>
#include <util-file.h>
#include <util-mem.h>
#include <util-std.h>
#include <win-wait.h>

#define KEY_DELIM   L"\n"
#define KEY_MAX     32

// static prototypes
static void Defaults(void);
static BOOL File(const BOOL load);
static void Keys(const BOOL encrypt);
static BOOL Logging(const BOOL load);
static BOOL Start(void);
static void Stop(void);

/* wWinMain */
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previnst, LPWSTR cmd, int show)
{
    if(Logging(TRUE))
    {
        LTRACE(logger, IDS_TRACE_ENTER, L"wWinMain()");

        WinInit(instance, previnst, cmd, show);

        // create window and loop it
        Dims(NULL, 0, NULL, &wnd.client);
        if(Window(APP_TITLE, wnd.client, NULL, &wnd))
        {
            WindowConfig(Start, NULL, Stop, &wnd);
            WindowFunc(WM_PAINT, (void*)Draw, &wnd);
            WindowFunc(WM_KEYDOWN, (void*)Input, &wnd);
            WindowFunc(WM_KEYUP, (void*)Input, &wnd);
            WindowIcon(IDI_PARKIT, &wnd);
            Loop(wnd);
        }

        LTRACE(logger, IDS_TRACE_EXIT, L"wWinMain()");
        Logging(FALSE);
    }

    return(0);
}

/* set up standard defaults */
static void Defaults(void)
{
    LTRACE(logger, IDS_TRACE_ENTER, L"Default()");

    MemClear(&app, sizeof(app));
    app.server = TRUE;
    app.showlegend = TRUE;

    LTRACE(logger, IDS_TRACE_EXIT, L"Defaults()");
}

/* attempt to read in or write out system-stored server configuration */
static BOOL File(const BOOL load)
{
    CFILE   cf = {0};
    wchar_t path[STR_PATH] = {0};
    BOOL    success = FALSE;

    LTRACE(logger, IDS_TRACE_ENTER, L"File()");

    // get system path for save data
    if(FolderApp(path, sizeof(path), L"%s", APP_PATH))
    {
        // open save data
        if(FileOpen((load ? FILE_READ : FILE_WRITE), path, &cf))
        {
            // iterate through each file_save or file_load call for demos
            if(load)
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

    LTRACE(logger, IDS_TRACE_EXIT, L"File()");
    return(success);
}

/* pull in all keys used to encrypt data on disk */
static void Keys(const BOOL encrypt)
{
    long    n = 0;
    wchar_t keys[KEY_MAX * STR_NORM] = {0};
    wchar_t temp[STR_NORM] = {0};

    LTRACE(logger, IDS_TRACE_ENTER, L"Keys()");

    if(Resource(RSC_TYPE_TXT, IDR_PARKIT_KEYS, keys, sizeof(keys)))
    {
        // text file containing data-at-rest passwords has been opened
        for(n = 0; (n < KEY_MAX); ++n)
        {
            // use each key to encrypt or decrypt the app data as appropriate
            if(StrParse(temp, sizeof(temp), keys, sizeof(keys), KEY_DELIM, (encrypt ? (n + 1) : (KEY_MAX - n))))
            {
                EncryptXOR(&app, sizeof(app), temp, StrLen(temp, sizeof(temp)));
            }
        }
    }

    LTRACE(logger, IDS_TRACE_EXIT, L"Keys()");
}

/* start up or shut down logging capabilites */
static BOOL Logging(const BOOL load)
{
    wchar_t path[STR_PATH] = {0};
    BOOL    success = FALSE;

    LTRACE(logger, IDS_TRACE_ENTER, L"Logging()");

    if(load)
    {
        // identify the path to log all application textual logs
        if(FolderApp(path, sizeof(path), L"%s", APP_LOG))
        {
            // create the logging object
            if(Log(path, &logger))
            {
                // pass the logging object to all DLLs which need it
                UtilLogger(logger);
                GphLogger(logger);
                NetLogger(logger);
                WinLogger(logger);
                success = TRUE;
            }
        }
    }else{
        // stop the logging process and perform cleanup
        success = LogKill(&logger);
    }

    LTRACE(logger, IDS_TRACE_EXIT, L"Logging()");
    return(success);
}

/* start application */
static BOOL Start(void)
{
    BOOL    success = FALSE;

    LTRACE(logger, IDS_TRACE_ENTER, L"Start()");

    // attempt to load system-stored server configuration data
    if(!File(TRUE))
    {
        // unable to load app data, set defaults
        Defaults();
    }

    // start up application logic
    if(Logic(TRUE))
    {
        // start up database operations
        if(Database(TRUE))
        {
            // start up network operations
            if(Network(TRUE))
            {
                // start up GUI
                success = Interface(TRUE);
            }
        }
    }

    LTRACE(logger, IDS_TRACE_EXIT, L"Start()");
    return(success);
}

/* stop application */
static void Stop(void)
{
    LTRACE(logger, IDS_TRACE_ENTER, L"Stop()");

    File(FALSE);
    Interface(FALSE);
    Network(FALSE);
    Database(FALSE);
    Logic(FALSE);
    WindowKill(&wnd);

    LTRACE(logger, IDS_TRACE_EXIT, L"Stop()");
}
