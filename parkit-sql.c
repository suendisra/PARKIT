/**
  @file     parkit-sql.c
  @brief    Source file for PARKIT application
  @author   suendisra
*/
#include "parkit.h"

#define SQL_DB_CATALOG  L"parkit"
#define SQL_DB_TABLE    L"lot-status"

static SQLIDX   sql = -1;

// static prototypes
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
                if(SqlConnect(sql, TRUE) == TRUE)
                {
                    SqlSource(sql, SQL_DB_CATALOG, SQL_DB_TABLE);
                    success = TRUE;
                }else{
                    /// TODO - instruct user to TRY AGAIN
                    MemClear(&app.db, sizeof(app.db));
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
