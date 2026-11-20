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
    PARK_MSG_DB_SNAPSHOT,
    PARK_MSG_SPOT,
    PARK_MSG_LAST
};

static SPOT empty = {0};

// static prototypes
static void NewClientAccepted(const NETIDX idx);
static void NewClientSnapshot(const KEYVALUE *kvpair, const void *userdata);
static void RecvSpot(const void *subdata, const ULONG sz);
static BOOL SendSpot(const void *userdata, void *pubdata);

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
            // configure the CAL for the application
            if(app.server)
            {
                CalTransactID(GetTransactionID);
                CalOnAccept(NewClientAccepted);
                CalPublisher(PARK_MSG_DB_SNAPSHOT, sizeof(SPOT), SendSpot);
            }
            CalPublisher(PARK_MSG_SPOT, sizeof(SPOT), SendSpot);
            CalSubscriber(PARK_MSG_DB_SNAPSHOT, RecvSpot);
            CalSubscriber(PARK_MSG_SPOT, RecvSpot);

            // start CAL network operations
            success = CalStart();
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

/* new client was accepted by server; pass it the latest DB snapshot */
static void NewClientAccepted(const NETIDX idx)
{
    INDEX   eID = 0;

    if(app.server && (idx > 0))
    {
        // send client DB snapshot
        MapForEach(spots, NULL, NewClientSnapshot);
        CalPublish(PARK_MSG_DB_SNAPSHOT, &eID);
    }
}

/* iterate through map and send out each spot data to new client */
static void NewClientSnapshot(const KEYVALUE *kvpair, const void *userdata)
{
    if((kvpair != NULL) && (userdata == NULL))
    {
        CalPublish(PARK_MSG_DB_SNAPSHOT, &kvpair->key);
    }
}

/* receive published message from client/server to update given spot with given information */
static void RecvSpot(const void *subdata, const ULONG sz)
{
    SPOT    *sp = ((SPOT*)subdata);

    LTRACE(logger, IDS_TRACE_ENTER, L"RecvUpdateSpot()");

    if((sp != NULL) && (sz == sizeof(SPOT)))
    {
        // update map with received spot information
        if(sp->spot == 0)
        {
            // this is an empty spot to signal end of spot sending (utilized by DB snapshots)
            CalSubscriber(PARK_MSG_DB_SNAPSHOT, NULL);
        }else{
            // push into valid spot
            if(MapPush(spots, &sp->spot, sizeof(sp->spot), sp, sizeof(*sp)) && app.server)
            {
                // update database with relavent information
                UpdateSpot(*sp);
            }
        }
    }

    LTRACE(logger, IDS_TRACE_EXIT, L"RecvUpdateSpot()");
}

/* publish message to client/server to update given spot with given information */
static BOOL SendSpot(const void *userdata, void *pubdata)
{
    INDEX   index = *((INDEX*)userdata);
    SPOT    *sp = ((SPOT*)pubdata);
    BOOL    success = FALSE;

    LTRACE(logger, IDS_TRACE_ENTER, L"SendUpdateSpot()");

    if((userdata != NULL) && (pubdata != NULL))
    {
        // place spot information into publisher data buffer
        if(index == 0)
        {
            // this is an empty spot to signal end of spot sending (utilized by DB snapshots)
            sp = &empty;
            success = TRUE;
        }else{
            // pop the valid spot and send out
            success = MapPop(spots, sp, &index, sizeof(index));
        }
    }

    LTRACE(logger, IDS_TRACE_EXIT, L"SendUpdateSpot()");
    return(success);
}
