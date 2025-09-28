/**
  @file     parkit-client.c
  @brief    Source file for PARKIT application
  @author   suendisra
*/
#include "parkit.h"

#define ERROR_CONNECT   L"An error was encountered while attempting to connect to server over TCP. Please ensure correct configuration and try again."
#define MUTEX_WAIT      -1

static SPOT     *spots = NULL;
static long     scount = 0;

static INDEX    mut = -1;
static INDEX    net = -1;

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
            // set up UDP listening server to obtain TCP connection information
            success = NetServer(disc);
        }
    }else{
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
        NetEvent(disc, NULL, NULL, UdpRead, NULL, NULL);
        NetEvent(host, NULL, ServerConnect, ServerRead, NULL, ServerClose);
    }
}

/* based on current spot information, toggle if spot is overriden */
void OverrideSpot(const long index)
{
    if(index >= 0)
    {
        /// TODO
    }
}

/* prompt client user to give reservation info, then send to server */
void ReserveSpot(const long index)
{
    if(index >= 0)
    {
        /// TODO
    }
}

/* server closed its socket, perform clean up */
static void ServerClose(const NETIDX serv)
{
    NetClose(serv);
    app.net.connected = FALSE;
    Refresh(wnd.handl);
}

/* asynchronous connection attempt has returned, handle it */
static void ServerConnect(const NETIDX serv, const long err)
{
    if(serv >= 0)
    {
        if(err == 0)
        {
            // successful connection
            app.net.connected = TRUE;
            Refresh(wnd.handl);
        }else{
            // ran into trouble
            Message(MSG_ERR, wnd.handl, L"%s", ERROR_CONNECT);
        }
    }
}

/* server has sent data, read it in */
static void ServerRead(const NETIDX serv)
{
    PACKET  pack = {0};

    if(NetRecv(serv, &pack, sizeof(pack), NULL) == TRUE)
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
BOOL SpotInfo(const long index, SPOT *spot)
{
    void    *ptr = NULL;
    BOOL    got = FALSE;

    if((spot != NULL) && (Clamped(index, -1, scount) == TRUE))
    {
        // block until we gain control of the mutex
        if(MutexLock(mut, MUTEX_WAIT) == TRUE)
        {
            // copy out requested data
            ptr = (void*)(((BYTE*)spots) + (index * sizeof(*spot)));
            MemCopy(spot, ptr, sizeof(*spot));

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
    SOCKADDR_IN from = {0};
    wchar_t     cmp[STR_NORM] = {0};
    wchar_t     ip[STR_NORM] = {0};
    wchar_t     msg[STR_NORM] = {0};

    if((serv >= 0) && (app.net.connected == FALSE))
    {
        // receive packet from UDP discovery
        if(NetRecv(serv, msg, sizeof(msg), &from) == TRUE)
        {
            // ensure proper version match
            StrFormat(cmp, sizeof(cmp), L"%s%s", APP_TITLE, APP_VERSION);
            if(StrCompare(msg, cmp, 0, FALSE) == 0)
            {
                // convert address to IP
                if(InetNtop(AF_INET, &from, ip, (sizeof(ip) / sizeof(ip[0]))) != NULL)
                {
                    // attempt to create TCP/IP address for received address
                    if(NetAddrIP(host, ip, from.sin_port) == TRUE)
                    {
                        // attempt connection
                        NetConnect(serv);
                    }
                }
            }
        }
    }
}
