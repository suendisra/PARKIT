/**
  @file     parkit-gui.c
  @brief    Source file for PARKIT application
  @author   suendisra
*/
#include "parkit.h"

#define FONT_NAME                   L"Consolas"
#define FONT_SIZE                   24

#define INFO_COLS                   8
#define INFO_CLIENT                 L"[L]EGEND| | | | | |[M]ODE |SRV(%ld)|"
#define INFO_ROWS                   1
#define INFO_SERVER                 L"[L]EGEND| | | | | |[M]ODE |DB(%ld)|"

#define RATIO_LEGEND                0.55    // divisional value for legend overlap display
#define RATIO_SCREEN                0.96    // divisional value for main screen and info bar

#define SCREEN_COLS                 8
#define SCREEN_ROUND                10
#define SCREEN_ROWS                 8

#define SPOT_INFO                   L"%ld-%03ld\nUntil: %s\nPlate: %s"

static GPH      gph = {0};
static GRID     screen = {0};
static GRID     info = {0};
static INDEX    font = -1;
static CLRPOOL  scolor = {0};
static long     coloroffset = 0;

// static prototypes
static void DrawInfo(void);
static void DrawLegend(void);
static void DrawScreen(void);

/* draw primary application screen */
void Draw(const HWND parent)
{
    if(GphPaint(&gph, parent) == TRUE)
    {
        DrawInfo();
        DrawScreen();

        // if user has opted for it, or first time starting up (ever), show display
        DrawLegend();

        // blit and display
        GphBlit(gph);
        GphPaint(&gph, parent);
    }
}

/* draw informational text regarding application states */
static void DrawInfo(void)
{
    wchar_t temp[STR_MEDIUM] = {0};

    // clear the background and info bar
    GphClear(gph, &info.dims, GCOAL);
    GphFontColor(gph, GGRAY, GTRANS);

    if(app.net.server == TRUE)
    {
        StrFormat(temp, sizeof(temp), INFO_SERVER, app.net.server);
    }else{
        StrFormat(temp, sizeof(temp), INFO_CLIENT, app.net.connected);
    }

    CharReplace(temp, sizeof(temp), CHARPIPE, CHARNULL);
    GphList(info, NULL, (ALIGN_LEFT | ALIGN_VCENTER), temp);
}

/* draw legend of colors to indicate properties of each parking spot portrayed */
static void DrawLegend(void)
{
    QUAD    dims = screen.dims;

    if(app.showlegend == TRUE)
    {
        // draw legend background
        QuadScale(TRUE, RATIO_LEGEND, RATIO_LEGEND, &dims);
        GphStock(gph, GGRAY, GWHITE);
        GphRect(gph, dims, SCREEN_ROUND);

        // TODO
    }
}

/* draw contents of each parking spot in the  */
static BOOL DrawParkingSpot(const QUAD cell, const long index)
{
    SPOT    spot = {0};
    QUAD    tdims = {0};
    long    idx = 0;
    long    cx = 0;
    long    cy = 0;
    wchar_t temp[STR_NORM] = {0};

    // retrieve information regarding this spot
    if(SpotInfo(index, TRUE, &spot) == TRUE)
    {
        // draw a border for this parking lot spot
        idx = ((spot.reserved == TRUE) ? coloroffset : 0);
        idx += ((spot.empty == TRUE) * 1);
        idx += ((spot.overridden == TRUE) * 2);
        idx += ((spot.expired == TRUE) * 4);

        // draw color-coded status background rect
        GphStock(gph, scolor.color[idx], GGRAY);
        GphRect(gph, cell, SCREEN_ROUND);

        // draw gray textual backdrop so text pops
        GphStock(gph, GGRAY, GGRAY);
        StrFormat(temp, sizeof(temp), SPOT_INFO, spot.level, spot.spot, L"", L"");
        FontDims(font, gph.mem.dc, &cx, &cy, L"%s", temp);
        QuadCentered(cell.mx, cell.my, cx, cy, &tdims);
        // TODO - GphRect(gph, tdims, SCREEN_ROUND);

        // draw text for the spot
        GphFontColor(gph, GBLACK, GTRANS);
        GphText(gph, &cell, ALIGN_CENTER2D, L"%s", temp);
    }else{
        // no information, draw empty spot
        GphStock(gph, GTRANS, GGRAY);
        GphRect(gph, cell, SCREEN_ROUND);
    }

    return(TRUE);
}

/* draw main application screen with parking lot information */
static void DrawScreen(void)
{
    // clear the background and info bar
    GphClear(gph, &screen.dims, GBLACK);
    GridFunc(screen, DrawParkingSpot);
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

                // create a grid for the textual information
                Grid(&gph, &dims[0], SCREEN_ROWS, SCREEN_COLS, HEADER_NONE, &screen);
                Grid(&gph, &dims[1], INFO_ROWS, INFO_COLS, HEADER_NONE, &info);

                // set up colors for quick and easy understanding of parking spot status
                PoolDefault(&scolor);
                coloroffset = (scolor.count / 2);
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
