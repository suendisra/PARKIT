/**
  @file     parkit-config.c
  @brief    Source file for PARKIT application
*/
#include "parkit.h"
#include <net-cal.h>
#include <util-mem.h>

enum PARKITMSGTYPE
{
    PARK_MSG_INVAL = -1,
    PARK_MSG_NEW_CLIENT,
    PARK_MSG_UPDATE_SPOT,
    PARK_MSG_LAST
};

// static prototypes
static BOOL RecvUpdateSpot(const void *subdata, const ULONG sz);
static BOOL SendUpdateSpot(const void *userdata, void *pubdata);

/* initialize or tear down winsock */
BOOL Network(const BOOL start)
{
    enum CALMODE    mode = (app.server ? CALMODE_SERVER : CALMODE_CLIENT);
    BOOL            success = FALSE;

    LTRACE(logger, IDS_TRACE_ENTER, L"Network()");

    if(start)
    {
        // start up network and CAL operations
        if(Net(TRUE) && Cal(mode, APP_TITLE))
        {
            // start the CAL
            if(CalStart())
            {
                CalTransactID(GetTransactionID);
                CalPublisher(PARK_MSG_UPDATE_SPOT, sizeof(SPOT), SendUpdateSpot);
                CalSubscriber(PARK_MSG_UPDATE_SPOT, RecvUpdateSpot);
            }
        }
    }else{
        // kill the network operations
        CalStop();
        CalKill();
        success = Net(FALSE);
    }

    LTRACE(logger, IDS_TRACE_EXIT, L"Network()");
    return(success);
}

/* receive published message from client/server to update given spot with given information */
static BOOL RecvUpdateSpot(const void *subdata, const ULONG sz)
{
    SPOT    *sp = ((SPOT*)subdata);
    BOOL    success = FALSE;

    LTRACE(logger, IDS_TRACE_ENTER, L"RecvUpdateSpot()");

    if((sp != NULL) && (sz == sizeof(SPOT)))
    {
        // update map with received spot information
        success = SpotInfo(sp->midx, FALSE, sp);
        if(success && app.server)
        {
            // update database with relavent information
            success &= UpdateSpot(*sp);
        }
    }

    LTRACE(logger, IDS_TRACE_EXIT, L"RecvUpdateSpot()");
    return(success);
}

/* publish message to client/server to update given spot with given information */
static BOOL SendUpdateSpot(const void *userdata, void *pubdata)
{
    INDEX   index = *((INDEX*)userdata);
    SPOT    *sp = ((SPOT*)pubdata);
    BOOL    success = FALSE;

    LTRACE(logger, IDS_TRACE_ENTER, L"SendUpdateSpot()");

    if((userdata != NULL) && (pubdata != NULL))
    {
        // place spot information into publisher data buffer
        success = SpotInfo(index, TRUE, sp);
    }

    LTRACE(logger, IDS_TRACE_EXIT, L"SendUpdateSpot()");
    return(success);
}
