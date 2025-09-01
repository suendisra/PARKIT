/**
  @file     parkit.h
  @brief    Header file for PARKIT application
  @author   suendisra
  @version  0.1.3
*/
#ifndef _THREADS_H_
#define _THREADS_H_

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
#define APP_VERSION         L"0.0.0"

// include needed libraries
#pragma comment(lib, "UTIL.lib")
#pragma comment(lib, "GPH.lib")
#pragma comment(lib, "NET.lib")
#pragma comment(lib, "WIN.lib")

typedef struct
{
    long    level;
    long    spot;
}PACKET;

struct
{
    BOOL    server;
}app;

WNDW    wnd;

/**
  @fn       void Heartbeat(void)
  @brief    send out UDP broadcast for clients to discover and connect
*/
void Heartbeat(void);

/**
  @fn       BOOL StartClient(void)
  @brief    start application as a client
  @param    startup if TRUE, start the client, if FALSE, destroy
  @return   TRUE if setup/destruction was successful, FALSE otherwise
*/
BOOL Client(const BOOL startup);

/**
  @fn       BOOL StartServer(void)
  @brief    start application as a server 
  @param    startup if TRUE, start the server, if FALSE, destroy
  @return   TRUE if setup/destruction was successful, FALSE otherwise
*/
BOOL Server(const BOOL startup);

#endif
