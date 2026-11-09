/**
  @file     parkit-gui.c
  @brief    Source file for PARKIT application
*/
#include "parkit.h"
#include <gph-font.h>
#include <gph-grid.h>
#include <gph-shape.h>

#define FONT_NAME                   L"Consolas"
#define FONT_SIZE                   24

#define INFO_COLS                   8
#define INFO_CLIENT                 L"[L]EGEND| | | | | | | |"
#define INFO_ROWS                   1
#define INFO_SERVER                 L"[L]EGEND| | | | | | |DB(%ld)|"

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
    LTRACE(logger, IDS_TRACE_ENTER, L"Draw()");

    if(GphPaint(gph, parent))
    {
        DrawInfo();
        DrawScreen();

        // if user has opted for it, or first time starting up (ever), show display
        DrawLegend();

        // blit and display
        GphBlit(gph);
        GphPaint(gph, parent);
    }

    LTRACE(logger, IDS_TRACE_EXIT, L"Draw()");
}

/* draw informational text regarding application states */
static void DrawInfo(void)
{
    wchar_t temp[STR_MEDIUM] = {0};

    LTRACE(logger, IDS_TRACE_ENTER, L"DrawInfo()");

    // clear the background and info bar
    GphClear(gph, &info.dims, GCOAL);
    GphFontColor(gph, GGRAY, GTRANS);

    if(app.server)
    {
        StrFormat(temp, sizeof(temp), INFO_SERVER, app.server);
    }else{
        StrFormat(temp, sizeof(temp), INFO_CLIENT);
    }

    CharReplace(temp, sizeof(temp), CHARPIPE, CHARNULL);
    GridList(info, NULL, (ALIGN_LEFT | ALIGN_VCENTER), temp);

    LTRACE(logger, IDS_TRACE_EXIT, L"DrawInfo()");
}

/* draw legend of colors to indicate properties of each parking spot portrayed */
static void DrawLegend(void)
{
    QUAD    dims = screen.dims;

    LTRACE(logger, IDS_TRACE_ENTER, L"DrawLegend()");

    if(app.showlegend)
    {
        // draw legend background
        QuadScale(TRUE, RATIO_LEGEND, RATIO_LEGEND, &dims);
        GphStock(gph, GGRAY, GWHITE);
        GphRect(gph, dims, SCREEN_ROUND);

        // TODO
    }

    LTRACE(logger, IDS_TRACE_EXIT, L"DrawLegend()");
}

/* draw contents of each parking spot in the  */
static BOOL DrawParkingSpot(const QUAD cell, const INDEX index, void *userdata)
{
    SPOT    spot = {0};
    long    idx = 0;

    LTRACE(logger, IDS_TRACE_ENTER, L"DrawParkingSpot()");

    if((cell.cx > 0) && (cell.cy > 0) && (index >= 0) && (userdata == NULL))
    {
        // retrieve information regarding this spot
        if(SpotInfo(index, TRUE, &spot))
        {
            // draw a border for this parking lot spot
            idx = (spot.reserved ? coloroffset : 0);
            idx += (spot.empty * 1);
            idx += (spot.overridden * 2);
            idx += (spot.expired * 4);

            // draw color-coded status background rect
            // todo - GphStock(gph, scolor.color[idx], GGRAY);
            GphRect(gph, cell, SCREEN_ROUND);

            // draw text for the spot
            GphFontColor(gph, GBLACK, GTRANS);
            GphText(gph, &cell, ALIGN_CENTER2D, SPOT_INFO, spot.level, index, L"", spot.vehicle);
        }else{
            // no information, draw empty spot
            GphStock(gph, GTRANS, GGRAY);
            GphRect(gph, cell, SCREEN_ROUND);
        }
    }

    LTRACE(logger, IDS_TRACE_EXIT, L"DrawParkingSpot()");
    return(TRUE);
}

/* draw main application screen with parking lot information */
static void DrawScreen(void)
{
    LTRACE(logger, IDS_TRACE_ENTER, L"DrawScreen()");

    // clear the background and info bar
    GphClear(gph, &screen.dims, GBLACK);
    GridFunc(screen, NULL, DrawParkingSpot);
    GphBlit(gph);

    LTRACE(logger, IDS_TRACE_EXIT, L"DrawScreen()");
}

/* initialize or tear down WinGDI */
BOOL Interface(const BOOL start)
{
    QUAD    dims[2] = {0};
    BOOL    success = FALSE;

    LTRACE(logger, IDS_TRACE_ENTER, L"Interface()");

    if(start)
    {
        // divide screen into main drawing area and informational bar
        QuadDivideH(wnd.client, RATIO_SCREEN, &dims[0], &dims[1]);

        // create the Windows GDI object
        if(Gph(wnd.handl, wnd.client, &gph))
        {
            // create primary font for display
            font = FontGDI(FONT_NAME, FONT_SIZE, FALSE, FALSE, FALSE);
            if(font >= 0)
            {
                // set the font text and color
                GphFontSet(gph, font);

                // create a grid for the textual information
                if(Grid(gph, &dims[0], &screen) && Grid(gph, &dims[1], &info))
                {
                    GridConfig(&screen, HEADER_NONE, SCREEN_ROWS, SCREEN_COLS);
                    GridConfig(&info, HEADER_NONE, INFO_ROWS, INFO_COLS);
                }

                // set up colors for quick and easy understanding of parking spot status
                Pool(&scolor);
                coloroffset = (PoolCount(scolor) / 2);
                success = TRUE;
            }
        }
    }else{
        // drop the font and Win GDI
        FontKillGDI(font);
        GphKill(&gph);
        success = TRUE;
    }

    LTRACE(logger, IDS_TRACE_EXIT, L"Interface()");
    return(success);
}
