/**
  @file     parkit-serv.c
  @brief    Source file for PARKIT application
  @author   suendisra
*/
#include "parkit.h"

#define ACTION_WAIT         500

#define HEARTBEAT_ELAPSE    2500
#define HEARTBEAT_TIMER     9750

#define SERVER_MAX_CLIENTS  64
#define SERVER_TCP_PORT     12600
#define SERVER_UDP_PORT     12613

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

static QUEUE    aq = {0};
static INDEX    at = -1;
static INDEX    nt = -1;

static NETIDX   heart = -1;
static NETIDX   serv = -1;

static NETIDX   clients[SERVER_MAX_CLIENTS] = {0};
static long     count = 0;

// static prototypes
static void ActionProc(const long thread);
static void ClientAccept(const NETIDX idx);
static void ClientRead(const NETIDX idx);
static void ClientClose(const NETIDX idx);
static void NetworkProc(const long thread);

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
                    clients[count] = a.idx;
                    count++;
                    break;

                case ACTION_CLOSE:
                    // client has closed, remove from tracking array
                    /// TODO
                    break;

                case ACTION_READ:
                    // process the data read from client
                    /// TODO
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

/* read from the client */
static void ClientRead(const NETIDX idx)
{
    ACTION  a = {0};
    PACKET  pack = {0};

    // receive the data from the client
    if(NetRecv(idx, &pack, sizeof(pack)) == TRUE)
    {
        // data read in, push into action queue for processing
        MemCopy(&a.pack, &pack, sizeof(a.pack));
        a.action = ACTION_READ;
        a.idx = idx;
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

/* send out UDP discovery information */
void Heartbeat(void)
{
    wchar_t msg[STR_SMALL] = {0};

    StrFormat(msg, sizeof(msg), L"%s%s", APP_TITLE, APP_VERSION);
    NetCastUDP(heart, SERVER_UDP_PORT, msg, sizeof(msg));
}

/* handle each network event that occurs */
static void NetworkProc(const long thread)
{
    if(thread >= 0)
    {
        NetEvent(serv, ClientAccept, NULL, ClientRead, NULL, ClientClose);
    }
}

/* start application as a server */
BOOL Server(const BOOL startup)
{
    NETIDX  inval = -1;
    long    n = 0;
    BOOL    success = FALSE;

    if(startup == TRUE)
    {
        // default all client sockets to an invalid state
        MemFill(clients, sizeof(clients), &inval, sizeof(inval));
        if(Queue(QUEUE_FIFO, sizeof(ACTION), &aq) == TRUE)
        {
            // spin up threads to listen for network events and handle actions on the queue
            at = Thread(ActionProc, NULL);
            nt = Thread(NetworkProc, NULL);
            if((at >= 0) && (nt >= 0))
            {
                // create a UDP socket for location broadcasting and TCP listening socket for client connections
                heart = NetOpen(NETSOCK_UDP, TRUE);
                serv = NetOpen(NETSOCK_TCP4, TRUE);
                if((heart >= 0) && (serv >= 0))
                {
                    if(Database(TRUE) == TRUE)
                    {
                        // start heartbeat, let others find ther server
                        Timer(wnd.handl, HEARTBEAT_TIMER, HEARTBEAT_ELAPSE);
                        /// TODO - change from NET_IP_HOME to NET_IP_LOCAL when ready to deploy to live environment
                        NetAddrIPV4(serv, NET_IP_HOME, SERVER_TCP_PORT);
                        success = NetServer(serv);
                    }
                }
            }
        }
    }else{
        // close any clients who are connected via TCP
        for(n = 0; (n < count); ++n)
        {
            NetClose(clients[n]);
        }

        // close listening socket and heartbeat UDP
        NetClose(serv);
        NetClose(heart);

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
