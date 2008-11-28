/*
  Name: extension.c
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 11/01/04 17:07
  Description: This is the extension manager, the extension manager enables
  the user and various user applications to customize various aspects of the
  operating system to their needs. It is also useful for testing out new
  OS algorithms and various other experiments.
  
    DEX educational extensible operating system 1.0 Beta
    Copyright (C) 2004  Joseph Emmanuel DL Dayo
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
*/

#include "dex32_devmgr.h"

#define EXT_MAXEXTENSIONS 50

#define CURRENT_SCHEDULER 1
#define CURRENT_MALLOC    2
#define CURRENT_VMM       3

typedef struct _extension_info {
DWORD deviceid;
devmgr_generic *iface;
} extension_info;

int extension_busy = 0 ,extension_ps_ready = 0;

extension_info *extension_table = 0;
