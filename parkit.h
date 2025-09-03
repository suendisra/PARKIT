/**
  @file     parkit.h
  @brief    Header file for PARKIT application
  @author   suendisra
  @version  0.1.4
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

#define APP_COPYRIGHT       2025
#define APP_PATH            L"parkit.dat"
#define APP_TITLE           L"PARKIT"
#define APP_VERSION         L"0.2.4"

// include needed libraries
#pragma comment(lib, "UTIL.lib")
#pragma comment(lib, "GPH.lib")
#pragma comment(lib, "NET.lib")
#pragma comment(lib, "WIN.lib")

typedef struct
{
    DATUM   dt;
}HEADER;

typedef struct
{
    HEADER  hdr;
    long    level;
    long    spot;
}PACKET;

struct
{
    struct
    {
        wchar_t inst[STR_NORM];
        wchar_t pwd[STR_NORM];
        wchar_t user[STR_NORM];
    }db;

    long    lots;
    BOOL    server;
}app;

WNDW    wnd;

/**
  @fn           BOOL Database(const BOOL standup)
  @brief        set up or tear down communications to the database
  @param[in]    startup if TRUE, start the database connections, if FALSE, destroy
  @return       TRUE if setup/destruction was successful, FALSE otherwise
*/
BOOL Database(const BOOL standup);

/**
  @fn           void Draw(const HWND parent)
  @brief        draw primary application screen
  @param[in]    parent handle to parent window that needs redrawing
*/
void Draw(const HWND parent);

/**
  @fn           void Heartbeat(void)
  @brief        send out UDP broadcast for clients to discover and connect
*/
void Heartbeat(void);

/**
  @fn           BOOL StartClient(const BOOL startup)
  @brief        start application as a client
  @param[in]    startup if TRUE, start the client, if FALSE, destroy
  @return       TRUE if setup/destruction was successful, FALSE otherwise
*/
BOOL Client(const BOOL startup);

/**
  @fn           BOOL Interface(const BOOL start)
  @brief        initialize or tear down WinGDI
  @param[in]    start if TRUE, start WinGDI operations, FALSE otherwise
*/
BOOL Interface(const BOOL start);

/**
  @fn           BOOL StartServer(const BOOL startup)
  @brief        start application as a server 
  @param[in]    startup if TRUE, start the server, if FALSE, destroy
  @return       TRUE if setup/destruction was successful, FALSE otherwise
*/
BOOL Server(const BOOL startup);

#endif
