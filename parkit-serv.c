/**
  @file     parkit-serv.c
  @brief    Source file for PARKIT application
  @author   suendisra
*/
#include "parkit.h"

#define ACTION_WAIT     500

// actions which the action thread will perform
enum ACTIONTYPE
{
    ACTION_INVAL = -1,
    ACTION_ACCEPT,
    ACTION_CLOSE,
    ACTION_READ,
    ACTION_WRITE,
    ACTION_LAST
};

// action information the thread will use
typedef struct
{
    enum ACTIONTYPE action;
    NETIDX          client;
    PACKET          pack;
}ACTION;

static QUEUE        aq = {0};
static INDEX        at = -1;
static INDEX        nt = -1;
static MAP          clients = {0};

// static prototypes
static void ActionProc(const long thread);
static void ClientAccept(const NETIDX idx);
static void ClientRead(const NETIDX idx);
static void ClientClose(const NETIDX idx);
static void NetworkProc(const long thread);
static void ProcessAccept(const NETIDX idx);
static void ProcessClose(const NETIDX idx);
static void ProcessRead(const NETIDX idx, const PACKET *p);
static void ProcessWrite(const NETIDX idx, const SPOT spot);

/* perform each action on the queue */
static void ActionProc(const long thread)
{
    ACTION  a = {0};

    // iterate thread while it is still alive
    while(ThreadCancelled(thread) == FALSE)
    {
        // pop each action while there are actions to perform
        while(QueuePop(&a, &aq) == TRUE)
        {
            switch(a.action)
            {
                case ACTION_ACCEPT:
                    // new client was accepted, place into tracking array
                    ProcessAccept(a.client);
                    break;

                case ACTION_CLOSE:
                    // client has closed, remove from tracking array
                    ProcessClose(a.client);
                    break;

                case ACTION_READ:
                    // process the data read from client
                    ProcessRead(a.client, &a.pack);
                    break;

                case ACTION_WRITE:
                    // write data on socket to client
                    ProcessWrite(a.client, a.pack.data.spot);
                    break;
            }
        }

        Sleep(ACTION_WAIT);
    }
}

/* accept the client */
static void ClientAccept(const NETIDX idx)
{
    ACTION  a = {0};

    // attempt to accept the client
    a.client = NetAccept(idx);
    if(a.client >= 0)
    {
        // accepted, push into action queue to for client tracking
        a.action = ACTION_ACCEPT;
        QueuePush(&a, &aq);
    }
}

/* client closed */
static void ClientClose(const NETIDX idx)
{
    ACTION  a = {0};

    // close the client, remove from tracking
    if(NetClose(idx) == TRUE)
    {
        a.action = ACTION_CLOSE;
        a.client = idx;
        QueuePush(&a, &aq);
    }
}

/* read from the client */
static void ClientRead(const NETIDX idx)
{
    ACTION  a = {0};
    PACKET  pack = {0};

    // receive the data from the client
    if(NetRecv(idx, &pack, sizeof(pack), NULL) == TRUE)
    {
        // decrypt incoming client data
        if(EncryptXOR(&pack, sizeof(pack), app.net.pwd, app.net.plen) == TRUE)
        {
            // verify data passed is valid
            if(VerifyPacket(&pack) == TRUE)
            {
                // data read in, push into action queue for processing
                MemCopy(&a.pack, &pack, sizeof(a.pack));
                a.action = ACTION_READ;
                a.client = idx;
                QueuePush(&a, &aq);
            }
        }
    }
}

/* send out UDP discovery information */
void DiscoverMe(void)
{
    PACKET  pkt = {0};

    // configure broadcast packet
    Datum(DATUMZONE_UTC, &pkt.dt);
    StrCopy(pkt.data.cast.title, sizeof(pkt.data.cast.title), APP_TITLE, 0);
    StrCopy(pkt.data.cast.version, sizeof(pkt.data.cast.version), APP_VERSION, 0);
    pkt.data.cast.port = app.net.tcp;
    pkt.type = PKT_TYPE_CAST;

    // configure and encrypt UDP broadcast data
    if(EncryptXOR(&pkt, sizeof(pkt), app.net.pwd, app.net.plen) == TRUE)
    {
        NetCastUDP(disc, &pkt, sizeof(pkt));
    }
}

/* handle each network event that occurs */
static void NetworkProc(const long thread)
{
    if(thread >= 0)
    {
        NetEvent(host, ClientAccept, NULL, ClientRead, NULL, ClientClose);
    }
}

/* new client has been accepted by the server, perform handshake */
static void ProcessAccept(const NETIDX idx)
{
    PACKET  pkt = {0};
    BOOL    valid = FALSE;

    if(idx >= 0)
    {
        // store client socket in map, indicating that client still isn't valid until after handshake
        if(MapPush(&clients, &idx, &valid) == TRUE)
        {
            // build packet for handshake
            Datum(DATUMZONE_UTC, &pkt.dt);
            pkt.type = PKT_TYPE_HANDSHAKE;
            pkt.data.shake.ack = FALSE;
            pkt.data.shake.req = TRUE;
            pkt.id = idx;

            // encrypt handshake packet
            if(EncryptXOR(&pkt, sizeof(pkt), app.net.pwd, app.net.plen) == TRUE)
            {
                // send client handshake request
                NetSend(idx, &pkt, sizeof(pkt));
            }
        }
    }
}

/* client closed socket, process clean up */
static void ProcessClose(const NETIDX idx)
{
    BOOL    valid = FALSE;

    if(idx >= 0)
    {
        // remove client from map and close its socket
        MapPop(&clients, &idx, &valid);
        NetClose(idx);
    }
}

/* based off of packet contents, perform requested operations */
static void ProcessRead(const NETIDX idx, const PACKET *p)
{
    LPVOID  found = NULL;
    BOOL    valid = TRUE;

    if((idx >= 0) && (p != NULL))
    {
        switch(p->type)
        {
            case PKT_TYPE_HANDSHAKE:
                // client was accepted, but has now returned the handshake
                if(MapFind(&clients, &p->id) != NULL)
                {
                    // now client is considered valid
                    MapPush(&clients, &p->id, &valid);
                }
                break;

            case PKT_TYPE_SPOTINFO:
                // ensure spot information data only comes from valid client
                found = MapFind(&clients, &p->id);
                if(found != NULL)
                {
                    // check client validity
                    valid = *(BOOL*)found;
                    if(valid == TRUE)
                    {
                        // valid client, pass updated data to database
                        if(UpdateSpot(p->data.spot) == TRUE)
                        {
                            // update server's local copy of the information
                            if(SpotInfo(FALSE, (SPOT*)&p->data.spot) == TRUE)
                            {
                                /// TODO - update all other clients with spot info (essentially stack action queue with commands to processwrite)
                                // spin up actions to inform all clients
                            }
                        }
                    }
                }
                break;
        }
    }
}

/* based off of packet contents, perform requested operations */
static void ProcessWrite(const NETIDX idx, const SPOT spot)
{
    PACKET  pkt = {0};

    if(idx >= 0)
    {
        // prepare outgoing packet
        Datum(DATUMZONE_UTC, &pkt.dt);
        pkt.type = PKT_TYPE_SPOTINFO;
        MemCopy(&pkt.data.spot, &spot, sizeof(pkt.data.spot));

        // encrypt outgoing packet
        if(EncryptXOR(&pkt, sizeof(pkt), app.net.pwd, app.net.plen) == TRUE)
        {
            NetSend(idx, &pkt, sizeof(pkt));
        }
    }
}

/* start application as a server */
BOOL Server(const BOOL startup)
{
    NETIDX      client = 0;
    long        n = 0;
    BOOL        success = FALSE;

    if(startup == TRUE)
    {
        // concurrent FIFO queue will handle actions requested by clients
        if(Queue(QUEUE_FIFO, sizeof(ACTION), &aq) == TRUE)
        {
            // spin up threads to listen for network events and handle actions on the queue
            at = Thread(ActionProc, NULL);
            nt = Thread(NetworkProc, NULL);
            if((at >= 0) && (nt >= 0))
            {
                // set up client-side behavior too
                Client(TRUE, TRUE);

                // establish connection to the primary database
                if(Database(TRUE) == TRUE)
                {
                    if(FullPollDB() == TRUE)
                    {
                        Timer(wnd.handl, TIMER_DISCOVERY_PULSE, TIMER_DISCOVERY_ELAPSE);
                        success = TRUE;
                    }
                }
            }
        }
    }else{
        // stop broadcasting
        Timer(wnd.handl, TIMER_DISCOVERY_PULSE, 0);

        // close any clients who are connected via TCP
        for(n = (clients.cc - 1); (n >= 0); --n)
        {
            if(MapPop(&clients, &n, &client) == TRUE)
            {
                NetClose(client);
            }
        }

        // destroy queue and its consumer thread
        ThreadKill(nt, FALSE);
        ThreadKill(at, FALSE);
        QueueKill(&aq);

        // close DB connection
        Database(FALSE);
        success = TRUE;
    }

    return(success);
}
