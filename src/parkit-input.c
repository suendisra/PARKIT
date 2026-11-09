/**
  @file     parkit-sql.c
  @brief    Source file for PARKIT application
*/
#include "parkit.h"
#include <util-mem.h>

static KEYBOARD kb = {0};

/* handle user input */
void Input(const WORD key, const BOOL down)
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
