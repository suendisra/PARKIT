/**
  @file     parkit-config.c
  @brief    Source file for PARKIT application
  @author   suendisra
*/
#include "parkit.h"

#define CONFIG_ERROR    L"An error was encountered while attempting to stand up network operations. Please adjust network configuration values and try again."
#define CONFIG_PASSWORD L"The server password for communications must not be empty. Please specify a password."
#define CONFIG_PORTS    L"One or both of the ports given are invalid. Please specify ports in the range of (%ld, %ld)."

// static prototypes
static void ListIP(const INDEX idx, const wchar_t *ip);
static INT_PTR CALLBACK NetworkConfigProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

/* place each IP address found from NetAddrIP into list */
static void ListIP(const INDEX idx, const wchar_t *ip)
{
    ListInsert(dlg.handl, IDC_IP_LIST, idx, L"%s", ip);
}

/* initialize or tear down winsock */
BOOL Network(const BOOL start)
{
    BOOL    success = FALSE;

    if(start == TRUE)
    {
        // start up winsock
        if(Net(TRUE) == TRUE)
        {
            // if a server, disc is used for discovery broadcasting
            // if a client, disc is used for listening to UDP discovery broadcasting
            disc = NetOpen(NETSOCK_UDP, TRUE);

            // if a server, host is the server socket that will accept incoming client connections
            // if a client, host is the client socket that will connect to the server
            host = NetOpen(NETSOCK_TCP4, TRUE);

            if((disc >= 0) && (host >= 0))
            {
                // if bad ports or IP address, prompt for network configuration
                if((app.net.tcp == 0) || (app.net.udp == 0) || (app.net.pwd[0] == CHARNULL) || (app.net.ip[0] == CHARNULL))
                {
                    // run network configuration dialog
                    MemClear(&dlg, sizeof(dlg));
                    if(Dialog(NetworkConfigProc, IDD_NET_CONFIG, wnd.handl, &dlg) == TRUE)
                    {
                        Loop(&dlg);
                    }
                }

                // if valid parameters, attempt to stand up as server and/or client
                if((app.net.tcp > 0) && (app.net.udp > 0) && (app.net.pwd[0] != CHARNULL) && (app.net.ip[0] != CHARNULL))
                {
                    // set in the ip addresses and ports
                    NetAddrIP(disc, app.net.ip, app.net.udp);
                    NetAddrIP(host, app.net.ip, app.net.tcp);

                    // attempt to stand up
                    success = ((app.net.server == TRUE) ? Server(TRUE) : Client(TRUE));
                    if(success == FALSE)
                    {
                        // notify user of failure
                        Message(MSG_ERR, wnd.handl, L"%s", CONFIG_ERROR);

                        // force bad ip to re-enter config dialog on startup
                        app.net.ip[0] = CHARNULL;
                    }
                }
            }
        }
    }else{
        // shut down winsock
        Client(FALSE);
        if(app.net.server == TRUE)
        {
            Server(FALSE);
        }

        NetClose(disc);
        NetClose(host);
        Net(FALSE);
    }

    return(success);
}

/**
  @fn           static INT_PTR CALLBACK NetworkConfigProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
  @brief        Main window procedure
  @param[in]    hwnd handle to window whose proc is being processed
  @param[in]    msg windows message to be processed
  @param[in]    wp wparam associated with the message
  @param[in]    lp lparam associated with the message
  @return       system return value
*/
static INT_PTR CALLBACK NetworkConfigProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    long    rval = 0;
    wchar_t temp[STR_NORM] = {0};

    switch(msg)
    {
        case WM_INITDIALOG:
            // center on main window
            Center(hwnd, 0, NULL);
            dlg.handl = hwnd;

            // list available IP addresses
            NetAddrLocalIP(host, FALSE, ListIP);
            ListSelect(hwnd, IDC_IP_LIST, 0, 0);

            // set in current port values and stored password
            TextSet(hwnd, IDC_SERV_TCP_PORT, L"%ld", app.net.tcp);
            TextSet(hwnd, IDC_SERV_UDP_PORT, L"%ld", app.net.udp);
            TextSet(hwnd, IDC_SERV_PASSWORD, L"%s", app.net.pwd);

            // enable or disable server controls as appropriate
            CheckDlgButton(hwnd, IDC_RUN_SERVER, ((app.net.server == TRUE) ? BST_CHECKED : BST_UNCHECKED));
            Enable(hwnd, IDC_STATIC_SERV_TCP_PORT, app.net.server);
            Enable(hwnd, IDC_SERV_TCP_PORT, app.net.server);
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
                case IDC_RUN_SERVER:
                    app.net.server = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_RUN_SERVER));
                    Enable(hwnd, IDC_STATIC_SERV_TCP_PORT, app.net.server);
                    Enable(hwnd, IDC_SERV_TCP_PORT, app.net.server);
                    break;

                case IDOK:
                    // pull out the TCP port number
                    TextGet(hwnd, IDC_SERV_TCP_PORT, temp, sizeof(temp));
                    Convert(CONVERT_STR_LNG, &app.net.tcp, temp, sizeof(app.net.tcp));

                    // pull out the UDP port number
                    TextGet(hwnd, IDC_SERV_UDP_PORT, temp, sizeof(temp));
                    Convert(CONVERT_STR_LNG, &app.net.udp, temp, sizeof(app.net.udp));

                    if((Clamped(app.net.tcp, NET_MIN_IPV_PORT, NET_MAX_IPV_PORT) == FALSE) || (Clamped(app.net.udp, NET_MIN_IPV_PORT, NET_MAX_IPV_PORT) == FALSE))
                    {
                        Message(MSG_ERR, hwnd, CONFIG_PORTS, NET_MIN_IPV_PORT, NET_MAX_IPV_PORT);
                    }else{
                        // pull out the server password for clients to be approved to connect
                        if(TextGet(hwnd, IDC_SERV_PASSWORD, app.net.pwd, sizeof(app.net.pwd)) == FALSE)
                        {
                            Message(MSG_ERR, hwnd, L"%s", CONFIG_PASSWORD);
                        }else{
                            // pull out the IP address and indicate if we're running as server or not
                            app.net.server = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_RUN_SERVER));
                            if(ListItem(hwnd, IDC_IP_LIST, LISTIDX_SEL, app.net.ip, sizeof(app.net.ip)) == TRUE)
                            {
                                // success, close window
                                SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
                            }
                        }
                    }
                    break;

                case IDCANCEL:
                    SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
                    break;
            }
            break;
    }

    return(rval);
}
