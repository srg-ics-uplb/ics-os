/*
  Name: Device Filesystem (For UNIX compatibility purposes)
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 19/01/00 05:37
  Description: This module mounts the block devices in the device manager to a directory
  on the VFS.
  
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
#ifdef DEVFS_H
#define DEVFS_H

#include "../dextypes.h"

DWORD devfs_getbytesperblock(int id);
void  devfs_rewritefile(vfs_node *f,int id);
int   devfs_writefile(vfs_node *f, char *buf, int start, int end, int device_id);
int   devfs_openfile(vfs_node *f, char *buf, DWORD start, DWORD end, int device_id);
int   devfs_mountroot(vfs_node *mountpoint,int device_id);
void  devfs_initnull();
void  devfs_init();

#endif
