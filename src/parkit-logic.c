/**
  @file     parkit-logic.c
  @brief    Source file for PARKIT application
*/
#include "parkit.h"

static MAP  spots = NULL;

/* start or stop application logic handling */
BOOL Logic(const BOOL load)
{
    BOOL    success = FALSE;

    if(load)
    {
        if(Map(&spots))
        {
            success = TRUE;
        }
    }else{
        MapKill(&spots);
        success = TRUE;
    }

    return(success);
}

/* return to caller information regarding the spot identified by the index */
BOOL SpotInfo(const INDEX index, const BOOL get, SPOT *spot)
{
    BOOL    success = FALSE;

    if((spot != NULL) && (index >= 0))
    {
        if(get)
        {
            // pull spot information from the map
            success = MapPop(spots, spot, &index, sizeof(index));
        }else{
            // put spot information into the map
            success = MapPush(spots, &index, sizeof(index), spot, sizeof(*spot));
        }
    }

    return(success);
}
