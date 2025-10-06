/**
  @file     parkit-sql.c
  @brief    Source file for PARKIT application
  @author   suendisra
*/
#include "parkit.h"

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

static SQLIDX   sql = -1;

static SPOT     sp = {0};

// static prototypes
static BOOL ProcessPollResults(const long row, const long col, const wchar_t *cell, void *userdata);
static INT_PTR CALLBACK SqlConfigProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

/* set up connection to the sql db server */
BOOL Database(const BOOL standup)
{
    BOOL    success = FALSE;

    if(standup == TRUE)
    {
        // if database connection hasn't been configured, prompt user to configure it
        if(app.db.inst[0] == CHARNULL)
        {
            if(Dialog(SqlConfigProc, IDD_SQL_CONFIG, wnd.handl, &dlg) == TRUE)
            {
                Loop(&dlg);
            }
        }

        // attempt connection with given (or stored) parameters
        if(app.db.inst[0] != CHARNULL)
        {
            sql = Sql(wnd.handl, app.db.inst, app.db.user, app.db.pwd);
            if(sql >= 0)
            {
                // attempt to connect to SQL database
                if(SqlConnect(sql, TRUE) == TRUE)
                {
                    SqlSource(sql, SQL_DB_CATALOG, SQL_DB_TABLE);
                    success = TRUE;
                }else{
                    // clear out database configuration and instruct user to try again
                    MemClear(&app.db, sizeof(app.db));
                    Message(MSG_ERR, wnd.handl, L"%s", ERROR_CONFIG);
                }
            }
        }
    }else{
        // destroy sql connection
        SqlConnect(sql, FALSE);
        SqlKill(sql);
        success = TRUE;
    }

    return(success);
}

/* retrieve all database values for parking lot statuses */
BOOL FullPollDB(void)
{
    BOOL    success = FALSE;

    // perform stored procedure to get all parking lot information
    if(SqlProc(sql, STORED_PROC_FULL_POLL) == TRUE)
    {
        // loop through returned data
        if(SqlLoop(sql, NULL, TRUE, ProcessPollResults) == TRUE)
        {
            // instruct window to redraw
            SendMessage(wnd.handl, WM_PAINT, 0, 0);
            success = TRUE;
        }
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
    // ignore user data passed in
    if(userdata != NULL){}

    // perform parameter checks
    if((row >= 0) && (col >= 0))
    {
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
    }

    return(TRUE);
}

/**
  @fn           static INT_PTR CALLBACK SqlConfigProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
  @brief        Main window procedure
  @param[in]    hwnd handle to window whose proc is being processed
  @param[in]    msg windows message to be processed
  @param[in]    wp wparam associated with the message
  @param[in]    lp lparam associated with the message
  @return       system return value
*/
static INT_PTR CALLBACK SqlConfigProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    long    rval = 0;

    switch(msg)
    {
        case WM_INITDIALOG:
            // center on main window
            Center(hwnd, 0, NULL);

            // set in whatever textual values are already there
            TextSet(hwnd, IDC_INSTANCE, L"%s", app.db.inst);
            TextSet(hwnd, IDC_CATALOG, L"%s", SQL_DB_CATALOG);
            TextSet(hwnd, IDC_TABLE, L"%s", SQL_DB_TABLE);
            TextSet(hwnd, IDC_USER, L"%s", app.db.user);
            TextSet(hwnd, IDC_PASSWORD, L"%s", app.db.pwd);
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

        case WM_COMMAND:
            switch(LOWORD(wp))
            {
                case IDOK:
                    // pull out the configuration values and allow connection attempt
                    TextGet(hwnd, IDC_INSTANCE, app.db.inst, sizeof(app.db.inst));
                    TextGet(hwnd, IDC_USER, app.db.user, sizeof(app.db.user));
                    TextGet(hwnd, IDC_PASSWORD, app.db.pwd, sizeof(app.db.pwd));

                    // intentional fall-through to close window
                case IDCANCEL:
                    SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
                    break;
            }
            break;
    }

    return(rval);
}

/* update database information for given spot */
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
