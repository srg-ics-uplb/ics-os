/*
  Name: Dex32 Task Manager
  Copyright: 
  Author: Joseph Emmanuel Dayo  
  Date: 16/11/03 04:34
  Description: A kernel module used to monitor and control processes and threads that
               are running. Activated by pressing ALT-F1
               
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

DEX32_DDL_INFO *taskmgrout; //holds the handle to the task managers console screen
int taskmgr_fgid;
fg_processinfo *taskmgrfg;
DWORD oldkeybfocus,tm_pid=0;

int dex32_tm_active=0;


void dex32_tm_updateinfo();

int dex32_get_tm_state()
{
  return dex32_tm_active;
};

fg_processinfo *tm_before;

void dex32_set_tm_state(int state)
{
  static int tm_before;
  
  dex32_tm_active = state;
  if (state) 
          tm_before=fg_setforeground(taskmgr_fgid);
       else
          fg_setforeground(tm_before);
};

void dex32_tm_showhelp()
{
        Dex32Clear(taskmgrout);
        Dex32SetTextBackground(taskmgrout, BLUE);
        Dex32SetTextColor(taskmgrout,WHITE);
        DDLprintf(&taskmgrout,"%-79s\n","Dex32- Task Manager v 1.00- HELP");
        Dex32SetTextBackground(taskmgrout, BLACK);
        DDLprintf(&taskmgrout,"This is the Dex32 realtime process monitor. This module enables you\n");
        DDLprintf(&taskmgrout,"monitor the status of the DEX kernel.\n");
        DDLprintf(&taskmgrout,"\n\n\npress any key to return.\n");
        getch();
};

void dex32_tm_updateinfo()
{
int refreshrate=100; //determines how long in milliseconds until the task manager refreshes its display
int loop;

 taskmgrout = Dex32CreateDDL();
 taskmgrfg = fg_register(taskmgrout,tm_pid);
 taskmgrfg->ignore = 1;
 taskmgr_fgid = taskmgrfg->id;
 Dex32SetProcessDDL(taskmgrout,tm_pid);;
 

 while(1)
    {
        dd_swaptomemory(taskmgrout);
        clrscr();
        textbackground(BLUE);
        textcolor(WHITE);
        printf("%-79s\n","Dex32- Realtime Monitor v 1.00");
        textbackground(BLACK);
        //Display processes in memory
        if (dex32_tm_active)
        show_process();
        dd_swaptohardware(taskmgrout);
        delay(refreshrate);
    };
};
