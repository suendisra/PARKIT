/**
  @file     parkit-sql.c
  @brief    Source file for PARKIT application
*/
#include "parkit.h"
#include <net-sql.h>
#include <util-mem.h>

#define ERROR_CONFIG                L"An error was encountered while attempting to connect to database via ODBC2. Please ensure correct configuration and try again."

#define SQL_DB_CATALOG              L"parkit"
#define SQL_DB_TABLE                L"lot-status"

#define STORED_PROC_FULL_POLL       L"FullDatabasePoll"
#define STORED_PROC_UPDATE_SPOT     L"UpdateParkingSpot"

enum TABLECOLUMN
{
    COLUMN_INVAL = -1,
    COLUMN_ID,
    COLUMN_LVL,
    COLUMN_SPOT,
    COLUMN_EMPTY,
    COLUMN_OVERRIDEN,
    COLUMN_SENSOR,
    COLUMN_RESERVE,
    COLUMN_EXPIRES,
    COLUMN_VEHICLE,
    COLUMN_LAST
};

static SQL  sql = NULL;
static WNDW dlg = {0};

// static prototypes
static BOOL DbCommand(const HWND hwnd, const WPARAM wp, const LPARAM lp);
static BOOL DbConnect(void);
static BOOL DbStart(void);
static BOOL ProcessPollResults(const long row, const long col, const wchar_t *cell, void *userdata);

/* set up connection to the sql db server */
BOOL Database(const BOOL standup)
{
    BOOL    success = FALSE;

    if(standup)
    {
        // if database connection hasn't been configured, prompt user to configure it
        if((app.server || app.backup) && (app.db.inst[0] == CHARNULL))
        {
            if(Dialog(IDD_CONFIG, wnd.handl, &dlg))
            {
                WindowConfig(DbStart, NULL, NULL, &dlg);
                WindowFunc(WM_COMMAND, (void*)DbCommand, &dlg);
                Loop(dlg);
            }
        }

        // all is configured, attempt the connection
        success = DbConnect();
    }else{
        // destroy sql connection
        SqlConnect(sql, FALSE);
        SqlKill(&sql);
        success = TRUE;
    }

    return(success);
}

/* handle command events for the database-network parameters dialog */
static BOOL DbCommand(const HWND hwnd, const WPARAM wp, const LPARAM lp)
{
    BOOL    checked = FALSE;

    if((hwnd != NULL) && (wp > 0) && (lp >= 0))
    {
        switch(LOWORD(wp))
        {
            case IDC_RUN_AS_SERVER:
                // only allow backup status if also acting as a server
                checked = ButtonChecked(hwnd, IDC_RUN_AS_SERVER);
                ButtonCheck(hwnd, IDC_RUN_AS_BACKUP, app.backup);
                Enable(hwnd, IDC_RUN_AS_BACKUP, checked);
                Enable(hwnd, IDC_INSTANCE, checked);
                Enable(hwnd, IDC_USER, checked);
                Enable(hwnd, IDC_PASSWORD, checked);
                break;

            case IDOK:
                // pull out the configuration values and allow connection attempt
                app.server = ButtonChecked(hwnd, IDC_RUN_AS_SERVER);
                app.backup = ButtonChecked(hwnd, IDC_RUN_AS_BACKUP);
                TextGet(hwnd, IDC_INSTANCE, app.db.inst, sizeof(app.db.inst));
                TextGet(hwnd, IDC_USER, app.db.user, sizeof(app.db.user));
                TextGet(hwnd, IDC_PASSWORD, app.db.pwd, sizeof(app.db.pwd));

                // intentional fall-through
            case IDCANCEL:
                Quit(hwnd);
                break;
        }
    }

    return(TRUE);
}

/* attempt connection with given (or stored) parameters */
static BOOL DbConnect(void)
{
    BOOL    success = !app.server;

    if((app.server || app.backup) && (app.db.inst[0] != CHARNULL))
    {
        if(Sql(wnd.handl, app.db.inst, &sql))
        {
            // attempt to connect to SQL database
            SqlConfig(&sql, app.db.user, app.db.pwd);
            if(SqlConnect(sql, TRUE))
            {
                SqlSource(sql, SQL_DB_CATALOG, SQL_DB_TABLE);
                success = FullPollDB();
            }else{
                // clear out database configuration and instruct user to try again
                MemClear(&app.db, sizeof(app.db));
                Message(MSG_ERR, wnd.handl, L"%s", ERROR_CONFIG);
            }
        }
    }

    return(success);
}

/* configure database and network parameters dialog with given values */
static BOOL DbStart(void)
{
    ButtonCheck(wnd.handl, IDC_RUN_AS_SERVER, app.server);
    ButtonCheck(wnd.handl, IDC_RUN_AS_BACKUP, app.backup);
    TextSet(wnd.handl, IDC_INSTANCE, L"%s", app.db.inst);
    TextSet(wnd.handl, IDC_CATALOG, L"%s", SQL_DB_CATALOG);
    TextSet(wnd.handl, IDC_TABLE, L"%s", SQL_DB_TABLE);
    TextSet(wnd.handl, IDC_USER, L"%s", app.db.user);
    TextSet(wnd.handl, IDC_PASSWORD, L"%s", app.db.pwd);
    return(TRUE);
}

/* retrieve all database values for parking lot statuses */
BOOL FullPollDB(void)
{
    BOOL    success = FALSE;

    // perform stored procedure to get all parking lot information
    if(SqlProc(sql, STORED_PROC_FULL_POLL, NULL, ProcessPollResults))
    {
        Refresh(wnd.handl);
        success = TRUE;
    }

    return(success);
}

/**
  @fn           static BOOL ProcessPollResults(const long row, const long col, const wchar_t *cell, void *userdata)
  @brief        dataset loop handler to process each piece of data returned from DB
  @param[in]    row index of row fetched by sql stored procedure
  @param[in]    col index of column data retrieved for each row
  @param[in]    cell data associated with the record data located at (row, col)
  @param[in]    userdata caller data passed to this function
  @return       true if data was successfully processed and loop should continue, false otherwise
*/
static BOOL ProcessPollResults(const long row, const long col, const wchar_t *cell, void *userdata)
{
    // perform parameter checks
    if((row >= 0) && (col >= 0) && (cell != NULL) && (userdata == NULL))
    {
/*
        switch(col)
        {
            case COLUMN_ID:
                // first column, reset spot parameters
                MemClear(&sp, sizeof(sp));
                sp.index = row;
                break;

            case COLUMN_LVL:
                // level on which spot is spot
                Convert(CONVERT_STR_LNG, &sp.level, cell, sizeof(sp.level));
                break;

            case COLUMN_SPOT:
                // spot on each level
                Convert(CONVERT_STR_LNG, &sp.spot, cell, sizeof(sp.spot));
                break;

            case COLUMN_EMPTY:
                // indicates whether spot on the level is empty or not
                sp.empty = (cell[0] == L'1');
                break;

            case COLUMN_OVERRIDEN:
                // indicates whether spot on the level was overridden or not
                sp.overridden = (cell[0] == L'1');
                break;

            case COLUMN_SENSOR:
                // indicates whether spot on the level has a sensor or not
                sp.sensor = (cell[0] == L'1');
                break;

            case COLUMN_RESERVE:
                // this is the reservation date for the parking spot
                if((cell != NULL) && (cell[0] != CHARNULL))
                {
                    DatumString(&sp.reserve, TRUE, (wchar_t*)cell, (STR_SMALL * sizeof(wchar_t)));
                    sp.reserved = TRUE;
                }
                break;

            case COLUMN_EXPIRES:
                // this is the expiration date to the reservation for the parking spot
                if((cell != NULL) && (cell[0] != CHARNULL))
                {
                    // TODO
                    OutputDebugString(cell);
                    OutputDebugString(L"\n");
                }
                break;

            case COLUMN_VEHICLE:
                // information on the vehicle, best practice is license plate
                if((cell != NULL) && (cell[0] != CHARNULL))
                {
                    StrCopy(sp.vehicle, sizeof(sp.vehicle), cell, 0);
                }

                // last column information, set spot into map
                SpotInfo(FALSE, &sp);
                break;
        }
*/
    }

    return(TRUE);
}

/* update database information for given spot
BOOL UpdateSpot(const SPOT spot)
{
    BOOL    success = FALSE;

    // bind all the spot information values for stored proc execution
    SqlBindInput(sql, SQLBIND_LONG, (void*)&spot.level, sizeof(spot.level));
    SqlBindInput(sql, SQLBIND_LONG, (void*)&spot.spot, sizeof(spot.spot));
    SqlBindInput(sql, SQLBIND_BOOL, (void*)&spot.empty, sizeof(spot.empty));
    SqlBindInput(sql, SQLBIND_BOOL, (void*)&spot.overridden, sizeof(spot.overridden));
    SqlBindInput(sql, SQLBIND_BOOL, (void*)&spot.sensor, sizeof(spot.sensor));
    SqlBindInput(sql, SQLBIND_DATUM, (void*)&spot.reserve, sizeof(spot.reserve));
    SqlBindInput(sql, SQLBIND_DATUM, (void*)&spot.expires, sizeof(spot.expires));
    SqlBindInput(sql, SQLBIND_TEXT, (void*)spot.vehicle, sizeof(spot.vehicle));

    // execute stored procedure with given parameters
    if(SqlProc(sql, STORED_PROC_UPDATE_SPOT) == TRUE)
    {
        // instruct window to redraw
        SendMessage(wnd.handl, WM_PAINT, 0, 0);
        success = TRUE;
    }

    return(success);
}
*/
