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

    if(KeySet(key, down, &kb))
    {
        if(kb.up['L'] == TRUE)
        {
            app.showlegend ^= TRUE;
        }

        // allow user to change mode (client<->server) if desired
        if(kb.up['M'] == TRUE)
        {
            // warn user that this shouldn't be done unless you're the network engineer
            // TODO

            // wants to proceed, so do so
            // TODO
        }

        // allow the user to exit the application just from Esc or 'X'
        if((kb.up['X'] == TRUE) || (kb.up[VK_ESCAPE] == TRUE))
        {
            SendMessage(wnd.handl, WM_SYSCOMMAND, SC_CLOSE, 0);
        }

        // redraw screen after user input
        if(input)
        {
            Refresh(wnd.handl);
        }

        // up-state of key(s) should not be persistent
        MemClear(&kb.up, sizeof(kb.up));
    }
}
