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
static BOOL ClientUpdateSpot(const void *userdata, void *data, ULONG *sz);

/* publish message to client to update given spot with given information */
static BOOL ClientUpdateSpot(const void *userdata, void *data, ULONG *sz)
{
    SPOT    *sp = NULL;
    INDEX   index = *((INDEX*)userdata);
    BOOL    success = FALSE;

    if((userdata != NULL) && (data != NULL) && (sz != NULL))
    {
        *sz = sizeof(SPOT);
        if(Mem(sizeof(SPOT), &data))
        {
            if(SpotInfo(index, TRUE, sp))
            {
            }
        }
    }

    return(success);
}

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
                CalPublisher(PARK_MSG_UPDATE_SPOT, ClientUpdateSpot);
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

/* ensure data packet is valid and from trusted source
BOOL VerifyPacket(const PACKET *pkt)
{
    DATUM   diff = {0};
    DATUM   now = {0};

    BOOL    verified = FALSE;

    if(pkt != NULL)
    {
        // ensure packet was sent within a reasonable time
        Datum(DATUMZONE_UTC, &now);
        DatumDiff(&now, &pkt->dt, &diff);
        if((diff.year == 0) && (diff.month == 0) && (diff.day == 0) && (diff.hour == 0) && (diff.min == 0) && (diff.sec < PACKET_EXPIRE))
        {
            // validate the data based on packet type
            switch(pkt->type)
            {
                case PKT_TYPE_CAST:
                    // ensure proper network port
                    if(Clamped(pkt->data.cast.port, NET_MIN_IPV_PORT, NET_MAX_IPV_PORT) == TRUE)
                    {
                        // ensure valid title and version
                        if((StrCompare(pkt->data.cast.title, APP_TITLE, 0, FALSE) == 0) && (StrCompare(pkt->data.cast.version, APP_VERSION, 0, FALSE) == 0))
                        {
                            verified = TRUE;
                        }
                    }
                    break;

                case PKT_TYPE_HANDSHAKE:
                    // packet data should be either request or acknowledgement
                    if(((pkt->data.shake.req == TRUE) && (pkt->data.shake.ack == FALSE)) || ((pkt->data.shake.req == FALSE) && (pkt->data.shake.ack == TRUE)))
                    {
                        verified = (pkt->id >= 0);
                    }
                    break;

                case PKT_TYPE_SPOTINFO:
                    if((pkt->data.spot.index >= 0) && (pkt->data.spot.level > 0) && (pkt->data.spot.spot > 0))
                    {
                        verified = TRUE;
                    }
                    break;
            }
        }
    }

    return(verified);
}
*/
