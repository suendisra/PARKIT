/**
  @file     parkit-sql.c
  @brief    Source file for PARKIT application
*/
#include "parkit.h"
#include <util-mem.h>
#include <gph-grid.h>
#include <win-prompt.h>

#define PROMPT_ADMIN_PASSWORD   L"Enter administrator password to change configuration:"

static WNDW dlg = {0};
static KEYBOARD kb = {0};

// static prototypes
static BOOL ConfigureSettings(void);
static void ConfigureSpot(const INDEX index);

/* prompt for admin password and if valid show configuration dialog */
static BOOL ConfigureSettings(void)
{
    wchar_t pwd[STR_NORM] = {0};
    BOOL    success = FALSE;

    // prompt for the admin password
    if(PromptLogin(wnd.handl, PROMPT_ADMIN_PASSWORD, NULL, 0, pwd, sizeof(pwd)) == RESULT_SUCCESS)
    {
        // todo - run db stored procedure to check password
        // todo - successful, display configuration window
        // todo - if successfully re-configured, stand up application under new configuration
    }

    return(success);
}

/* ask user for data regarding the spot, then publish to peers */
static void ConfigureSpot(const INDEX index)
{
    SPOT    spot = {0};

    if(index > 0)
    {
        // obtain the information regarding the spot
        if(MapPop(spots, &spot, &index, sizeof(index)))
        {
            // show the spot configuration dialog
            if(Dialog(IDD_SPOT, wnd.handl, &dlg))
            {
//todo                WindowConfig(DbStart, NULL, NULL, &dlg);
//todo                WindowFunc(WM_COMMAND, (void*)DbCommand, &dlg);
                WindowIcon(IDI_PARKIT, &dlg);
                Loop(dlg);
            }
        }
    }
}

/* handle user keyboard input */
void InputKey(const WORD key, const BOOL down)
{
    BOOL    input = FALSE;

    LTRACE(logger, IDS_TRACE_ENTER, L"Input()");

    if(KeySet(key, down, &kb))
    {
        // prompt for admin password; if valid, show configuration dialog
        if(kb.up['F'])
        {
            input = ConfigureSettings();
        }

        // toggle display of the legend text
        if(kb.up['L'])
        {
            app.showlegend ^= TRUE;
            input = TRUE;
        }

        // allow the user to exit the application just from Esc or 'X'
        if(kb.up['X'] || kb.up[VK_ESCAPE])
        {
            Quit(wnd.handl);
        }

        // redraw screen after user input
        if(input)
        {
            Refresh(wnd.handl);
        }

        // up-state of key(s) should not be persistent
        MemClear(&kb.up, sizeof(kb.up));
    }

    LTRACE(logger, IDS_TRACE_EXIT, L"Input()");
}

/* handle user mouse input */
void InputMouse(const MOUSE mo)
{
    long    c = -1;
    long    r = -1;

    if((mo.click.x >= 0) && (mo.click.y >= 0))
    {
        if(GridClick(screen, mo.click, &r, &c, NULL) >= 0)
        {
            if((mo.up[MOUSE_LEFT] > 0) && (r >= 0) && (c >= 0))
            {
                // clicked the grid, index obtained
                ConfigureSpot(SPOTID(app.level, r, c));
            }
        }
    }
}
