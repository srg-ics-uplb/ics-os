/*
  Name: DEX process dispatcher
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 23/01/04 04:53
  Description: This module is responsible for mapping processes from files, calling
  the appropriate moduleloader and starting them.
  
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

#define NEW_MODULE 0
#define FORK_MODULE 1


typedef struct _createp_queue
{
  int handle;
  int type;
  char *image;
  char *loadaddress;
  char name[255];
  char parameter[500];
  char workdir[500];
  int  mode;
  int parent;
  int dispatched;
  DWORD processid;
  struct _createp_queue *next;
} createp_queue;

createp_queue *pd_head=0;
DWORD pd_handlecounter=0;

int pd_busy=0,pd_ready=0;

int addmodule(char *name,char *image,char *loadaddress,int mode,char *parameter,char *workdir,int parent);
int pd_forkmodule(int parent);
int pd_ok(int handle);
