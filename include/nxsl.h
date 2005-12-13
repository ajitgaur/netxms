/* 
** NetXMS - Network Management System
** Copyright (C) 2005 Victor Kirhenshtein
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
** $module: nxsl.h
**
**/

#ifndef _nxsl_h_
#define _nxsl_h_

#ifdef _WIN32
#ifdef LIBNXSL_EXPORTS
#define LIBNXSL_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXSL_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXSL_EXPORTABLE
#endif


//
// Script handle
//

typedef void * NXSL_SCRIPT;


//
// Functions
//

#ifdef __cplusplus
extern "C" {
#endif

NXSL_SCRIPT LIBNXSL_EXPORTABLE NXSLCompile(TCHAR *pszSource,
                                           TCHAR *pszError, int nBufSize);
int LIBNXSL_EXPORTABLE NXSLRun(NXSL_SCRIPT hScript);
TCHAR LIBNXSL_EXPORTABLE *NXSLGetRuntimeError(NXSL_SCRIPT hScript);
void LIBNXSL_EXPORTABLE NXSLDestroy(NXSL_SCRIPT hScript);
void LIBNXSL_EXPORTABLE NXSLDump(NXSL_SCRIPT hScript, FILE *pFile);

#ifdef __cplusplus
}
#endif

#endif
