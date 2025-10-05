/**
  @file     parkit-client.c
  @brief    Source file for PARKIT application
  @author   suendisra
*/
#include "parkit.h"

#define ERROR_CONNECT           L"An error was encountered while attempting to connect to server over TCP. Please ensure correct configuration and try again."

#define MUTEX_WAIT              -1

// map to store all parking lot spot information
static MAP      spots;

static INDEX    mut = -1;
static INDEX    net = -1;
static INDEX    myID = -1;

// static prototypes
static long CompareIndex(const BYTE *b1, const BYTE *b2, const long sz);
static void NetworkProc(const long thread);
static void ProcessHandshake(const HAND shake);
static void ServerClose(const NETIDX serv);
static void ServerConnect(const NETIDX serv, const long err);
static void ServerRead(const NETIDX serv);
static void UdpRead(const NETIDX serv);

/* given two indicies, compare their values for searching and sorting in the map */
static long CompareIndex(const BYTE *b1, const BYTE *b2, const long sz)
{
    INDEX   i1 = -1;
    INDEX   i2 = -1;
    long    result = 0;

    if((b1 != NULL) && (b2 != NULL) && (sz > 0))
    {
        // pull out the index values
        i1 = *((INDEX*)b1);
        i2 = *((INDEX*)b2);
        if(i1 != i2)
        {
            // return comparison result
            result = ((i1 < i2) ? -1 : 1);
        }
    }

    return(result);
}

/* start application as a client */
BOOL Client(const BOOL startup, const BOOL forserver)
{
    BOOL    success = FALSE;

    if(startup == TRUE)
    {
        // create the map to store all parking lot information
        if(Map(sizeof(long), sizeof(SPOT), SPOT_BASE_COUNT, CompareIndex, &spots) == TRUE)
        {
            // spin up thread to handle network events for TCP or UDP
            mut = Mutex();
            if(forserver == FALSE)
            {
                net = Thread(NetworkProc, NULL);
                if((mut >= 0) && (net >= 0))
                {
                    // set up UDP listening server to obtain TCP connection information
                    success = NetServer(disc);
                }
            }else{
                success = TRUE;
            }
        }
    }else{
        ThreadKill(net, FALSE);
        MutexKill(mut);
        MapKill(&spots);
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

/* accept handshake from server and send back acknowledgement */
static void ProcessHandshake(const HAND shake)
{
    PACKET  pkt = {0};

    if((shake.req == TRUE) && (shake.ack == FALSE))
    {
        // server has requested handshake, send one back
        Datum(DATUMZONE_UTC, &pkt.dt);
        pkt.type = PKT_TYPE_HANDSHAKE;
        pkt.id = myID;
        pkt.data.shake.req = FALSE;
        pkt.data.shake.ack = TRUE;
        NetSend(host, &pkt, sizeof(pkt));
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
    if(serv == host)
    {
        if(err == 0)
        {
            // successful connection
            myID = -1;
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
    PACKET  pkt = {0};

    if(host == serv)
    {
        // receive and verify
        if(NetRecv(serv, &pkt, sizeof(pkt), NULL) == TRUE)
        {
            if(EncryptXOR(&pkt, sizeof(pkt), app.net.pwd, app.net.plen) == TRUE)
            {
                if(VerifyPacket(&pkt) == TRUE)
                {
                    // block until we gain control of the mutex
                    if(MutexLock(mut, MUTEX_WAIT) == TRUE)
                    {
                        // pull in the data from server, handle accordingly
                        switch(pkt.type)
                        {
                            case PKT_TYPE_HANDSHAKE:
                                myID = pkt.id;
                                ProcessHandshake(pkt.data.shake);
                                break;

                            case PKT_TYPE_SPOTINFO:
                                MapPush(&spots, &pkt.data.spot.index, &pkt.data.spot);
                                break;
                        }

                        // redraw window and release the mutex
                        Refresh(wnd.handl);
                        MutexUnlock(mut);
                    }
                }
            }
        }
    }
}

/* return to caller information regarding the spot identified by the index */
BOOL SpotInfo(const BOOL get, SPOT *spot)
{
    PACKET  pkt = {0};
    SPOT    *found = NULL;
    BOOL    success = FALSE;

    if((spot != NULL) && (spot->index >= 0))
    {
        // block until we gain control of the mutex
        if(MutexLock(mut, MUTEX_WAIT) == TRUE)
        {
            if(get == TRUE)
            {
                // copy out requested data
                found = (SPOT*)MapFind(&spots, &spot->index);
                if(found != NULL)
                {
                    MemCopy(spot, found, sizeof(*spot));
                    success = TRUE;
                }
            }else{
                // push spot information into map, send to server if we are client
                success = MapPush(&spots, &spot->index, spot);
                if((success == TRUE) && (app.net.server == FALSE))
                {
                    // prepare packet to send to server
                    Datum(DATUMZONE_UTC, &pkt.dt);
                    pkt.type = PKT_TYPE_SPOTINFO;
                    pkt.id = myID;
                    MemCopy(&pkt.data.spot, spot, sizeof(*spot));

                    // send it off
                    NetSend(host, &pkt, sizeof(pkt));
                }
            }

            // release the mutex
            MutexUnlock(mut);
        }
    }

    return(success);
}

/* read in the UDP socket information for connecting to server */
static void UdpRead(const NETIDX serv)
{
    PACKET      pkt = {0};
    SOCKADDR_IN from = {0};

    wchar_t     ip[STR_NORM] = {0};

    if((serv >= 0) && (app.net.connected == FALSE))
    {
        // receive and interpret broadcast message
        if(NetRecv(serv, &pkt, sizeof(pkt), &from) == TRUE)
        {
            // decrypt packet contents
            if(EncryptXOR(&pkt, sizeof(pkt), app.net.pwd, app.net.plen) == TRUE)
            {
                if(VerifyPacket(&pkt) == TRUE)
                {
                    // convert address to IP
                    if(InetNtop(AF_INET, &from, ip, (sizeof(ip) / sizeof(ip[0]))) != NULL)
                    {
                        // attempt to create TCP/IP address for received address
                        if(NetAddrIP(host, ip, pkt.data.cast.port) == TRUE)
                        {
                            // attempt connection
                            NetConnect(host);
                        }
                    }
                }
            }
        }
    }
}
