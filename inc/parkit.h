/**
  @file     parkit.h
  @brief    Header file for PARKIT application
*/
#ifndef _PARKIT_H_
#define _PARKIT_H_

// net includes must be first for proper compilation
#include <net-std.h>
#include <util-dtm.h>
#include <util-std.h>
#include <util-str.h>
#include <gph-grid.h>
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

#define APP_COPYRIGHT           2025
#define APP_LOG                 L"parkit.log"
#define APP_PATH                L"parkit.dat"
#define APP_TITLE               L"PARKIT"
#define APP_VERSION             L"0.2.7"

#define SPOTID(level, row, col) ((level * 10000) + ((row + 1) * 100) + (col + 1))

enum DRAWMODE
{
    DRAWMODE_INVAL = -1,
    DRAWMODE_DEFAULT,
    DRAWMODE_EDIT_SPOT,
    DRAWMODE_LAST
};

typedef struct
{
    ULONG   spot;

    BOOL    empty;
    BOOL    expired;
    BOOL    overridden;
    BOOL    reserved;
    BOOL    sensor;

    DATUM   reserve;
    DATUM   expires;
    DATUM   updated;

    wchar_t vehicle[STR_SMALL];
}SPOT;

struct
{
    struct
    {
        ULONGLONG   transactID;     // latest transaction ID for the parkit table
        wchar_t     inst[STR_NORM]; // database instance name
        wchar_t     pwd[STR_NORM];  // password to connect to database
        wchar_t     user[STR_NORM]; // username to connect to database
    }db;

    INDEX   level;
    BOOL    server;
    BOOL    showlegend;
}app;

WNDW    wnd;
GRID    screen;
MAP     spots;
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
  @fn               BOOL FullPollDB(void)
  @brief            retrieve all database values for parking lot statuses
  @return           TRUE if all data was successfully polled, FALSE otherwise
*/
BOOL FullPollDB(void);

/**
  @fn               void GetTransactionID(ULONGLONG *transactID)
  @brief            callback function to obtain transaction ID for heartbeat
  @param[in]        transactID pointer to value to set with the current transaction ID
*/
void GetTransactionID(ULONGLONG *transactID);

/**
  @fn               void Input(const WORD key, const BOOL down)
  @brief            handle user keyboard input
  @param[in]        wp wparam to WM_KEYDOWN and WM_KEYUP messages
  @param[in]        keyisdown if handling WM_KEYDOWN message, this is true, otherwise FALSE
*/
void InputKey(const WORD key, const BOOL down);

/**
  @fn               void InputMouse(const WORD button, const LPARAM lp, const BOOL down)
  @brief            handle user mouse input
  @param[in]        mo the MOUSE struct containing mouse input conditions
*/
void InputMouse(const MOUSE mo);

/**
  @fn               BOOL Interface(const BOOL start)
  @brief            initialize or tear down WinGDI
  @param[in]        start if TRUE, start WinGDI operations, FALSE otherwise
*/
BOOL Interface(const BOOL start);

/**
  @fn               void Keys(const BOOL encrypt, void *block, const long blocksz)
  @brief            pull in all keys used to encrypt data on disk
  @param[in]        encrypt if TRUE, encrypt block with passwords from resource file; if FALSE, decrypt
  @param[in out]    block data block to encrypt/decrypt
  @param[in]        blocksz size of data block to encrypt/decrypt
*/
void Keys(const BOOL encrypt, void *block, const long blocksz);

/**
  @fn               BOOL Network(const BOOL start)
  @brief            initialize or tear down winsock
  @param[in]        start if TRUE, start winsock operations, FALSE otherwise
*/
BOOL Network(const BOOL start);

/**
  @fn               BOOL UpdateSpot(const SPOT spot)
  @brief            updates database with given spot information
  @param[in]        spot information to update database with
  @return           TRUE if successfully updated database; FALSE otherwise
*/
BOOL UpdateSpot(const SPOT spot);

/**
  @fn               void UpdateTransactionID(void)
  @brief            increment the transaction ID as database transactions are performed
*/
void UpdateTransactionID(void);

#endif
