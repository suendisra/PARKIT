/**
  @file     parkit-sql.c
  @brief    Source file for PARKIT application
*/
#include "parkit.h"
#include <util-mem.h>
#include <gph-grid.h>

static KEYBOARD kb = {0};

/* handle user keyboard input */
void InputKey(const WORD key, const BOOL down)
{
    BOOL    input = FALSE;

    LTRACE(logger, IDS_TRACE_ENTER, L"Input()");

    if(KeySet(key, down, &kb))
    {
        if(kb.up['L'] == TRUE)
        {
            app.showlegend ^= TRUE;
            input = TRUE;
        }

        // allow the user to exit the application just from Esc or 'X'
        if((kb.up['X'] == TRUE) || (kb.up[VK_ESCAPE] == TRUE))
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
    INDEX   idx = -1;

    if((mo.click.x >= 0) && (mo.click.y >= 0))
    {
        idx = GridClick(screen, mo.click, NULL, NULL, NULL);
        if((mo.up[MOUSE_LEFT] > 0) && (idx >= 0))
        {
            // clicked the grid, index obtained
            // TODO
        }
    }
}
