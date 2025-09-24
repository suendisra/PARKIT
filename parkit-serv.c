/**
  @file     parkit-serv.c
  @brief    Source file for PARKIT application
  @author   suendisra
*/
#include "parkit.h"

#define ACTION_WAIT                 500

#define SERVER_MAX_CLIENTS          64
#define SERVER_TCP_PORT             12600

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

static NETIDX       heart = -1;
static NETIDX       serv = -1;
static long         port = -1;

static NETIDX       clients[SERVER_MAX_CLIENTS] = {0};
static ULONGLONG    clientID[SERVER_MAX_CLIENTS] = {0};
static long         count = 0;

// static prototypes
static void ActionProc(const long thread);
static void ClientAccept(const NETIDX idx);
static void ClientRead(const NETIDX idx);
static void ClientClose(const NETIDX idx);
static void NetworkProc(const long thread);
static void ProcessPacket(const NETIDX idx, const PACKET *p);
static BOOL ServerConfig(void);
static INT_PTR CALLBACK ServConfigProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
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
                    clients[count] = a.idx;
                    clientID[count] = a.pack.hdr.clientID;
                    count++;
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
    NetCastUDP(heart, msg, sizeof(msg));
}

/* handle each network event that occurs */
static void NetworkProc(const long thread)
{
    if(thread >= 0)
    {
        NetEvent(serv, ClientAccept, NULL, ClientRead, NULL, ClientClose);
    }
}

/* based off of packet contents, perform requested operations */
static void ProcessPacket(const NETIDX idx, const PACKET *p)
{
    /// TODO
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
                    // prompt user for desired server configuration
                    if(ServerConfig() == TRUE)
                    {
                        success = Database(TRUE);
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

/* set up connection to the sql db server */
static BOOL ServerConfig(void)
{
    BOOL    success = FALSE;

    // if server discovery udp port and client tcp port numbers haven't been configured, prompt user to configure it
    if(app.port == 0)
    {
        // randomize suggested TCP port for server configuration
        port = (long)Random(NET_MIN_IPV_PORT, NET_MAX_IPV_PORT);
        NetAddrLocal(serv, 0, port);

        // run the server configuration dialog window
        MemClear(&dlg, sizeof(dlg));
        if(Dialog(ServConfigProc, IDD_SERVER_CONFIG, wnd.handl, &dlg) == TRUE)
        {
            Loop(&dlg);
        }
    }

    // attempt connection with given (or stored) parameters
    if(app.port > 0)
    {
        // start heartbeat, let others find ther server
        Timer(wnd.handl, TIMER_HEARTBEAT_PULSE, TIMER_HEARTBEAT_ELAPSE);
        if(NetServer(serv) == TRUE)
        {
            success = TRUE;
        }else{
            // TODO - instruct user to TRY AGAIN
            MemClear(&app.port, sizeof(app.port));
        }
    }

    return(success);
}

/**
  @fn           static INT_PTR CALLBACK ServConfigProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
  @brief        Main window procedure
  @param[in]    hwnd handle to window whose proc is being processed
  @param[in]    msg windows message to be processed
  @param[in]    wp wparam associated with the message
  @param[in]    lp lparam associated with the message
  @return       system return value
*/
static INT_PTR CALLBACK ServConfigProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    long    rval = 0;
    wchar_t temp[STR_NORM] = {0};

    switch(msg)
    {
        case WM_INITDIALOG:
            // center on main window
            Center(hwnd, 0, NULL);

            // set in server discovery and connection port parameters
            NetIP(serv, temp, sizeof(temp), NULL);
            TextSet(hwnd, IDC_SERV_IP, L"%s", temp);
            TextSet(hwnd, IDC_SERV_TCP_PORT, L"%ld", port);
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
                    TextGet(hwnd, IDC_SERV_TCP_PORT, temp, sizeof(temp));
                    Convert(CONVERT_STR_LNG, &app.port, temp, sizeof(app.port));

                    // intentional fall-through to close window
                case IDCANCEL:
                    SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
                    break;
            }
            break;
    }

    return(rval);
}

/* ensure data packet is valid and from trusted source */
static BOOL VerifyData(const PACKET *p)
{
    BOOL    verified = FALSE;

    /// TODO
    verified = TRUE;

    return(verified);
}
