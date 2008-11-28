/*
  Name: foreground.c
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 05/01/04 04:39
  Description: Allows the management of multiple screens and controls the use of the
               foreground.
               
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
#include "dex_DDL.h"
#define FG_MAXCONSOLE 20


typedef struct _fg_processinfo {
int size;
int id;
int noswitch; // *NOT YET DONE* set to true to prevent CTRL+ALT+F5 and F6 to switch to this screen
DEX32_DDL_INFO *screen;
int keyboardfocus;
int ignore;
int pid; //process id of process that created this foreground info
struct _fg_processinfo *next,*prev;
} fg_processinfo;

fg_processinfo *fg_vconsoles[FG_MAXCONSOLE];
int fg_current = 0,fg_myslot = 0;
int fg_pid;

sync_sharedvar fg_busywait;

DEX32_DDL_INFO *fg_out;
fg_processinfo *fg_fginfo;

int fg_started = 0,fg_state = 0;

fg_processinfo *fg_getinfo(int pid);
int fg_getkeyboardowner();
fg_processinfo *fg_getmyinfo();
void fg_init();
void fg_setmykeyboard(int pid);
void fg_next();
void fg_prev();
fg_processinfo *fg_register(DEX32_DDL_INFO *scr, int keyboard);
int fg_setforeground(int num);
void fg_set_state(int state);
void fg_showmenu(int choice);
int fg_toggle();
void fg_updateinfo();

