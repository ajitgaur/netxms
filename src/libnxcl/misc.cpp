/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** $module: misc.cpp
**
**/

#include "libnxcl.h"


//
// Change library state and notify client
//

void ChangeState(DWORD dwState)
{
   g_dwState = dwState;
   CallEventHandler(NXC_EVENT_STATE_CHANGED, dwState, NULL);
}


//
// Create request for processing
//

HREQUEST CreateRequest(DWORD dwCode, void *pArg, BOOL bDynamicArg)
{
   REQUEST *pRequest;

   pRequest = (REQUEST *)MemAlloc(sizeof(REQUEST));
   pRequest->dwCode = dwCode;
   pRequest->pArg = pArg;
   pRequest->bDynamicArg = bDynamicArg;
   pRequest->dwHandle = g_dwRequestId++;
   g_pRequestQueue->Put(pRequest);
   return pRequest->dwHandle;
}


//
// Print debug messages
//

void DebugPrintf(char *szFormat, ...)
{
   va_list args;
   char *pBuffer;

   if (g_pDebugCallBack == NULL)
      return;

   pBuffer = (char *)MemAlloc(4096);
   va_start(args, szFormat);
   vsprintf(pBuffer, szFormat, args);
   va_end(args);
   g_pDebugCallBack(pBuffer);
   MemFree(pBuffer);
}
