/* 
** NetXMS subagent for IPSO
** Copyright (C) 2006 Alex Kirhenshtein
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
**/

#ifndef __IPSO_H__
#define __IPSO_H__


//
// File system stats
//

enum
{
	DISK_AVAIL,
	DISK_AVAIL_PERC,
	DISK_FREE,
	DISK_FREE_PERC,
	DISK_USED,
	DISK_USED_PERC,
	DISK_TOTAL,
};


//
// Memory stats
//

enum
{
	PHYSICAL_FREE,
	PHYSICAL_USED,
	PHYSICAL_TOTAL,
	SWAP_FREE,
	SWAP_USED,
	SWAP_TOTAL,
	VIRTUAL_FREE,
	VIRTUAL_USED,
	VIRTUAL_TOTAL,
};


//
// Handlers
//

LONG H_DiskInfo(char *, char *, char *);
LONG H_ProcessList(char *, char *, NETXMS_VALUES_LIST *);
LONG H_Uptime(char *, char *, char *);
LONG H_Uname(char *, char *, char *);
LONG H_Hostname(char *, char *, char *);
LONG H_CpuCount(char *, char *, char *);
LONG H_CpuLoad(char *, char *, char *);
LONG H_CpuUsage(char *, char *, char *);
LONG H_ProcessCount(char *, char *, char *);
LONG H_MemoryInfo(char *, char *, char *);

#endif /* __IPSO_H__ */
