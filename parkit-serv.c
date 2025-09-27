/**
  @file     parkit-serv.c
  @brief    Source file for PARKIT application
  @author   suendisra
*/
#include "parkit.h"

#define ACTION_WAIT                 500

#define SERVER_MAX_CLIENTS          64

// actions which the action thread will perform
enum ACTIONTYPE
{
    ACTION_INVAL = -1,
    ACTION_ACCEPT,
    ACTION_CLOSE,
    ACTION_READ,
    ACTION_LAST
};

// action information the thread will use
typedef struct
{
    enum ACTIONTYPE action;
    NETIDX          idx;
    PACKET          pack;
}ACTION;

static QUEUE        aq = {0};
static INDEX        at = -1;
static INDEX        nt = -1;

static struct
{
    NETIDX       sock;
    ULONGLONG    id;
}clients[SERVER_MAX_CLIENTS];

static long ccount = 0;

// static prototypes
static void ActionProc(const long thread);
static void ClientAccept(const NETIDX idx);
static void ClientRead(const NETIDX idx);
static void ClientClose(const NETIDX idx);
static void NetworkProc(const long thread);
static void ProcessPacket(const NETIDX idx, const PACKET *p);
static BOOL VerifyData(const PACKET *p);

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
                    clients[ccount].sock = a.idx;
                    clients[ccount].id = a.pack.hdr.clientID;
                    ccount++;
                    break;

                case ACTION_CLOSE:
                    // client has closed, remove from tracking array
                    /// TODO
                    break;

                case ACTION_READ:
                    // process the data read from client
                    ProcessPacket(a.idx, &a.pack);
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
    a.idx = NetAccept(idx);
    if(a.idx >= 0)
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
        a.idx = idx;
        QueuePush(&a, &aq);
    }
}

/* read from the client */
static void ClientRead(const NETIDX idx)
{
    ACTION  a = {0};
    PACKET  pack = {0};

    // receive the data from the client
    if(NetRecv(idx, &pack, sizeof(pack)) == TRUE)
    {
        if(VerifyData(&pack) == TRUE)
        {
            // data read in, push into action queue for processing
            MemCopy(&a.pack, &pack, sizeof(a.pack));
            a.action = ACTION_READ;
            a.idx = idx;
            QueuePush(&a, &aq);
        }
    }
}

/* send out UDP discovery information */
void DiscoverMe(void)
{
    wchar_t msg[STR_SMALL] = {0};

    StrFormat(msg, sizeof(msg), L"%s%s", APP_TITLE, APP_VERSION);
    NetCastUDP(disc, msg, sizeof(msg));
}

/* handle each network event that occurs */
static void NetworkProc(const long thread)
{
    if(thread >= 0)
    {
        NetEvent(host, ClientAccept, NULL, ClientRead, NULL, ClientClose);
    }
}

/* based off of packet contents, perform requested operations */
static void ProcessPacket(const NETIDX idx, const PACKET *p)
{
    if((idx >= 0) && (p != NULL))
    {
        /// TODO
    }
}

/* start application as a server */
BOOL Server(const BOOL startup)
{
    long    n = 0;
    BOOL    success = FALSE;

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
                // establish connection to the primary database
                Timer(wnd.handl, TIMER_DISCOVERY_PULSE, TIMER_DISCOVERY_ELAPSE);
                success = Database(TRUE);
            }
        }
    }else{
        // stop broadcasting
        Timer(wnd.handl, TIMER_DISCOVERY_PULSE, 0);

        // close any clients who are connected via TCP
        for(n = 0; (n < ccount); ++n)
        {
            NetClose(clients[n].sock);
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

/* ensure data packet is valid and from trusted source */
static BOOL VerifyData(const PACKET *p)
{
    BOOL    verified = FALSE;

    if(p != NULL)
    {
        /// TODO
        verified = TRUE;
    }

    return(verified);
}
