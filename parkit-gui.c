/**
  @file     parkit-gui.c
  @brief    Source file for PARKIT application
  @author   suendisra
*/
#include "parkit.h"

#define FONT_NAME                   L"Consolas"
#define FONT_SIZE                   24

#define INFO_COLS                   8
#define INFO_CLIENT                 L""
#define INFO_ROWS                   1
#define INFO_SERVER                 L" | | | | | | |DB(%ld)|"

#define SCREEN_COLS                 8
#define SCREEN_ROUND                10
#define SCREEN_ROWS                 12

#define RATIO_SCREEN                0.96    // divisional value for main screen and info bar

static GPH      gph = {0};
static GRID     screen = {0};
static GRID     info = {0};
static INDEX    font = -1;

// static prototypes
static void DrawInfo(void);
static void DrawScreen(void);

/* draw primary application screen */
void Draw(const HWND parent)
{
    if(GphPaint(&gph, parent) == TRUE)
    {
        DrawInfo();
        DrawScreen();
        GphPaint(&gph, parent);
    }
}

/* draw informational text regarding application states */
static void DrawInfo(void)
{
    wchar_t temp[STR_MEDIUM] = {0};

    // clear the background and info bar
    GphClear(gph, &info.dims, GCOAL);

    if(app.server == TRUE)
    {
        StrFormat(temp, sizeof(temp), INFO_SERVER, app.server);
    }else{
        /// TODO
    }
    CharReplace(temp, sizeof(temp), CHARPIPE, CHARNULL);
    GphList(info, NULL, (ALIGN_LEFT | ALIGN_VCENTER), temp);

    // blit and display
    GphBlit(gph);
}

/* draw main application screen with parking lot information */
static void DrawScreen(void)
{
    wchar_t temp[STR_XLARGE] = {0};

    // clear the background and info bar
    GphClear(gph, &screen.dims, GBLACK);

    // build the list of lots in the parking lot (show empty spots when none are present)
    /// TODO

    GphTiles(screen, NULL, SCREEN_ROUND, ALIGN_CENTER2D, temp);
    /// TODO

    // blit and display
    GphBlit(gph);
}

/* initialize or tear down WinGDI */
BOOL Interface(const BOOL start)
{
    QUAD    dims[2] = {0};
    BOOL    success = FALSE;

    if(start == TRUE)
    {
        // divide screen into main drawing area and informational bar
        QuadDivideH(wnd.client, RATIO_SCREEN, &dims[0], &dims[1]);

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

                // create a grid for the screen and for textual information
                Grid(&gph, &dims[0], SCREEN_ROWS, SCREEN_COLS, HEADER_NONE, &screen);
                Grid(&gph, &dims[1], INFO_ROWS, INFO_COLS, HEADER_NONE, &info);

                // call it good to go
                success = TRUE;
            }
        }
    }else{
        // drop the font and Win GDI
        FontKillGDI(font);
        GphKill(&gph);
        success = TRUE;
    }

    return(success);
}
