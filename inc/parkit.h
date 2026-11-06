/**
  @file     parkit.h
  @brief    Header file for PARKIT application
*/
#ifndef _PARKIT_H_
#define _PARKIT_H_

// net includes must be first for proper compilation
#include <net-std.h>
#include <util-str.h>
#include <win-std.h>
#include "resource.h"

// bring in appropriate version of library
#ifndef _DEBUG
#pragma comment(lib, "GPH.lib")
#pragma comment(lib, "NET.lib")
#pragma comment(lib, "UTIL.lib")
#pragma comment(lib, "WIN.lib")
#else
#pragma comment(lib, "GPHD.lib")
#pragma comment(lib, "NETD.lib")
#pragma comment(lib, "UTILD.lib")
#pragma comment(lib, "WIND.lib")
#endif

#define APP_COPYRIGHT   2025
#define APP_LOG         L"parkit.log"
#define APP_PATH        L"parkit.dat"
#define APP_TITLE       L"PARKIT"
#define APP_VERSION     L"0.2.7"

struct
{
    struct
    {
        wchar_t inst[STR_NORM]; // database instance name
        wchar_t pwd[STR_NORM];  // password to connect to database
        wchar_t user[STR_NORM]; // username to connect to database
    }db;

    BOOL    backup;
    BOOL    server;
    BOOL    showlegend;
}app;

WNDW    wnd;
LOGGER  logger;

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
  @fn               void Input(const WORD key, const BOOL down)
  @brief            handle user input
  @param[in]        wp wparam to WM_KEYDOWN and WM_KEYUP messages
  @param[in]        keyisdown if handling WM_KEYDOWN message, this is true, otherwise FALSE
*/
void Input(const WORD key, const BOOL down);

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
BOOL SpotInfo(const BOOL get, SPOT *spot);

/**
  @fn               BOOL VerifyPacket(const PACKET *p)
  @brief            pkt pointer to packet contents to verify
  @return           if true packet is valid, false otherwise
BOOL VerifyPacket(const PACKET *pkt);

/**
  @fn               BOOL UpdateSpot(const SPOT spot)
  @brief            update database information for given spot
  @param[in]        spot information for given spot
  @return           true if database was successfully updated, false otherwise
BOOL UpdateSpot(const SPOT spot);
*/

#endif
