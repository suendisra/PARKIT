/**
  @file     parkit-client.c
  @brief    Source file for PARKIT application
  @author   suendisra
*/
#include "parkit.h"

#define MUTEX_WAIT      -1

static SPOT     *spots = NULL;
static long     scount = 0;

static INDEX    mut = -1;
static INDEX    net = -1;
static NETIDX   sock = -1;
static NETIDX   udp = -1;

// static prototypes
static void NetworkProc(const long thread);
static void ServerClose(const NETIDX serv);
static void ServerConnect(const NETIDX serv, const long err);
static void ServerRead(const NETIDX serv);
static void UdpRead(const NETIDX serv);

/* start application as a client */
BOOL Client(const BOOL startup)
{
    BOOL    success = FALSE;

    if(startup == TRUE)
    {
        // spin up thread to handle network events for TCP or UDP
        mut = Mutex();
        net = Thread(NetworkProc, NULL);
        if((mut >= 0) && (net >= 0))
        {
            // open sockets for tcp and udp
            udp = NetOpen(NETSOCK_UDP, TRUE);
            sock = NetOpen(NETSOCK_TCP4, TRUE);
            if((udp >= 0) && (sock >= 0))
            {
                // set up UDP listening server to obtain TCP connection information
                if(NetServer(udp) == TRUE)
                {
                    success = TRUE;
                }
            }
        }
    }else{
        // stop client network operations
        NetClose(udp);
        NetClose(sock);
        MutexKill(mut);
        success = TRUE;
    }

    return(success);
}

/* handle each network event that occurs */
static void NetworkProc(const long thread)
{
    if(thread >= 0)
    {
        NetEvent(udp, NULL, NULL, UdpRead, NULL, NULL);
        NetEvent(sock, NULL, ServerConnect, ServerRead, NULL, ServerClose);
    }
}

/* based on current spot information, toggle if spot is overriden */
void OverrideSpot(const long index)
{
    /// TODO
}

/* prompt client user to give reservation info, then send to server */
void ReserveSpot(const long index)
{
    /// TODO
}

/* server close its socket, perform clean up */
static void ServerClose(const NETIDX serv)
{
    NetClose(serv);
    app.connected = FALSE;
    sock = -1;
    Refresh(wnd.handl);
}

/* asynchronous connection attempt has returned, handle it */
static void ServerConnect(const NETIDX serv, const long err)
{
    if(err == 0)
    {
        // successful connection
        app.connected = TRUE;
        Refresh(wnd.handl);
    }else{
        // ran into trouble
        /// TODO - let user know
    }
}

/* server has sent data, read it in */
static void ServerRead(const NETIDX serv)
{
    PACKET  pack = {0};
    void    *ptr = NULL;

    if(NetRecv(serv, &pack, sizeof(pack)) == TRUE)
    {
        // block until we gain control of the mutex
        if(MutexLock(mut, MUTEX_WAIT) == TRUE)
        {
            /// TODO read in the data and update the appropriate spot with the information

            // copy in supplied data
            /// TODO ptr = (spots + (index * sizeof(SPOT)));

            // redraw window and release the mutex
            Refresh(wnd.handl);
            MutexUnlock(mut);
        }
    }
}

/* return to caller information regarding the spot identified by the index */
BOOL SpotInfo(const long index, const SPOT *spot)
{
    void    *ptr = NULL;
    BOOL    got = FALSE;

    if((spot != NULL) && (Clamped(index, -1, scount) == TRUE))
    {
        // block until we gain control of the mutex
        if(MutexLock(mut, MUTEX_WAIT) == TRUE)
        {
            // copy out requested data
            ptr = (spots + (index * sizeof(SPOT)));
            MemCopy(spot, ptr, sizeof(SPOT));

            // release the mutex
            got = TRUE;
            MutexUnlock(mut);
        }
    }

    return(got);
}

/* read in the UDP socket information for connecting to server */
static void UdpRead(const NETIDX serv)
{
    if((serv >= 0) && (app.connected == FALSE))
    {
        /// TODO grab necessary data to perform TCP connection
        // attempt connection
        NetConnect(serv);
    }
}
