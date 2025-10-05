/**
  @file     parkit.h
  @brief    Header file for PARKIT application
  @author   suendisra
  @version  0.2.7
*/
#ifndef _PARKIT_H_
#define _PARKIT_H_

// net includes must be first for proper compilation
#include <net.h>
#include <net-sql.h>

#include <util.h>
#include <util-map.h>
#include <util-thread.h>

#include <gph.h>
#include <gph-font.h>

#include <win.h>

#include "resource.h"

#define APP_COPYRIGHT               2025
#define APP_PATH                    L"parkit.dat"
#define APP_TITLE                   L"PARKIT"
#define APP_VERSION                 L"0.2.7"

#define SPOT_BASE_COUNT             64

#define TIMER_DISCOVERY_ELAPSE      5000
#define TIMER_DISCOVERY_PULSE       9750

// include needed libraries
#pragma comment(lib, "UTIL.lib")
#pragma comment(lib, "GPH.lib")
#pragma comment(lib, "NET.lib")
#pragma comment(lib, "WIN.lib")

enum PKTTYPE
{
    PKT_TYPE_INVAL = -1,
    PKT_TYPE_CAST,
    PKT_TYPE_HANDSHAKE,
    PKT_TYPE_SPOTINFO,
    PKT_TYPE_LAST
};

// udp packet structure designed to hold broadcast information
typedef struct
{
    USHORT  port;
    wchar_t title[STR_SMALL];
    wchar_t version[STR_SMALL];
}CAST;

// struct for TCP server-client handshake
typedef struct
{
    BOOL    req;
    BOOL    ack;
}HAND;

// data for parking spot
typedef struct
{
    DATUM   reserve;
    DATUM   expires;
    BOOL    reserved;
    BOOL    expired;

    INDEX   index;
    long    level;
    long    spot;

    wchar_t vehicle[STR_MEDIUM];

    BOOL    empty;
    BOOL    overridden;
    BOOL    sensor;
}SPOT;

typedef struct
{
    enum PKTTYPE    type;

    DATUM           dt;
    INDEX           id;

    union
    {
        CAST    cast;
        SPOT    spot;
        HAND    shake;
    }data;
}PACKET;

struct
{
    struct
    {
        wchar_t inst[STR_NORM]; // database instance name
        wchar_t pwd[STR_NORM];  // password to connect to database
        wchar_t user[STR_NORM]; // username to connect to database
    }db;

    struct
    {
        USHORT  tcp;            // port number for TCP connections
        USHORT  udp;            // port number for UDP discovery broadcast

        wchar_t ip[STR_NORM];   // system IP address to use for tcp/udp
        wchar_t pwd[STR_NORM];  // password to connect to server (supplied by network admin during config)
        long    plen;

        BOOL    server;         // indicates if this device is running as server
        BOOL    connected;      // indicates if this device, as client, has connected to server
    }net;

    BOOL    showlegend;
}app;

WNDW    wnd;
WNDW    dlg;

// necessary sockets for network connections
NETIDX  disc;
NETIDX  host;

/**
  @fn               BOOL Database(const BOOL standup)
  @brief            set up or tear down communications to the database
  @param[in]        startup if TRUE, start the database connections, if FALSE, destroy
  @return           TRUE if setup/destruction was successful, FALSE otherwise
*/
BOOL Database(const BOOL standup);

/**
  @fn               void Draw(const HWND parent)
  @brief            draw primary application screen
  @param[in]        parent handle to parent window that needs redrawing
*/
void Draw(const HWND parent);

/**
  @fn               void Heartbeat(void)
  @brief            send out UDP broadcast for clients to discover and connect
*/
void DiscoverMe(void);

/**
  @fn               BOOL Client(const BOOL startup)
  @brief            start application as a client
  @param[in]        startup if TRUE, start the client, if FALSE, destroy
  @param[in]        forserver if on server side, allocate needed resources to also run client-side commands
  @return           TRUE if setup/destruction was successful, FALSE otherwise
*/
BOOL Client(const BOOL startup, const BOOL forserver);

/**
  @fn               BOOL FullPollDB(void)
  @brief            retrieve all database values for parking lot statuses
  @return           TRUE if all data was successfully polled, FALSE otherwise
*/
BOOL FullPollDB(void);

/**
  @fn               void Input(const WPARAM wp, const BOOL keyisdown)
  @brief            handle user input
  @param[in]        wp wparam to WM_KEYDOWN and WM_KEYUP messages
  @param[in]        keyisdown if handling WM_KEYDOWN message, this is true, otherwise FALSE
*/
void Input(const WPARAM wp, const BOOL keyisdown);

/**
  @fn               BOOL Interface(const BOOL start)
  @brief            initialize or tear down WinGDI
  @param[in]        start if TRUE, start WinGDI operations, FALSE otherwise
*/
BOOL Interface(const BOOL start);

/**
  @fn               BOOL Network(const BOOL start)
  @brief            initialize or tear down winsock
  @param[in]        start if TRUE, start winsock operations, FALSE otherwise
*/
BOOL Network(const BOOL start);

/**
  @fn               void OverrideSpot(const long index)
  @brief            based on current spot information, toggle if spot is overriden
  @param[in]        index based on mouse click location, calculated value of spot index
*/
void OverrideSpot(const long index);

/**
  @fn               void ReserveSpot(const long index)
  @brief            prompt client user to give reservation info, then send to server
  @param[in]        index based on mouse click location, calculated value of spot index
*/
void ReserveSpot(const long index);

/**
  @fn               BOOL Server(const BOOL startup)
  @brief            start application as a server 
  @param[in]        startup if TRUE, start the server, if FALSE, destroy
  @return           TRUE if setup/destruction was successful, FALSE otherwise
*/
BOOL Server(const BOOL startup);

/**
  @fn               BOOL SpotInfo(const BOOL get, SPOT *spot)
  @brief            return to caller information regarding the spot identified by the index
  @param[in]        get if TRUE, obtain spot information, if FALSE, push spot information
  @param[in out]    spot the information to pass back to caller
  @return           TRUE if information was retrieved, FALSE otherwise
*/
BOOL SpotInfo(const BOOL get, SPOT *spot);

/**
  @fn               BOOL VerifyPacket(const PACKET *p)
  @brief            pkt pointer to packet contents to verify
  @return           if true packet is valid, false otherwise
*/
BOOL VerifyPacket(const PACKET *pkt);

/**
  @fn               BOOL UpdateSpot(const SPOT spot)
  @brief            update database information for given spot
  @param[in]        spot information for given spot
  @return           true if database was successfully updated, false otherwise
*/
BOOL UpdateSpot(const SPOT spot);

#endif
