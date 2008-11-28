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

/*Initialize the virtual console table*/
void fg_init()
{
    int i;
    
    for (i=0; i < FG_MAXCONSOLE; i++)
    fg_vconsoles[i] = 0;
    
    
    memset(&fg_busywait,0,sizeof(fg_busywait));
};

int fg_toggle()
{
  fg_set_state(!fg_state);
};

int fg_setforeground(int num)
{
    DWORD cpuflags;
    int ret = -1;
    
    dex32_stopints(&cpuflags);
    
    if (num < FG_MAXCONSOLE)
    {
        if (fg_vconsoles[num]!=0)
        {
               DEX32_DDL_INFO *fgDDL= fg_vconsoles[num]->screen;
               int ret = fg_current;
               Dex32SetActiveDDL(fgDDL);
               fg_current = num;
        };
    };
    
    dex32_restoreints(cpuflags);
    
    return ret;
};


void fg_prev()
{
   int candidate = -1;
   int i;
   for (i= fg_current -1; i >= 0; i--)
      {
            if (fg_vconsoles[i]!=0)
               if (!fg_vconsoles[i]->ignore)
                {candidate = i;break;};
      };   
   if (candidate != -1) fg_setforeground(candidate);
};

void fg_next()
{
   int candidate = -1;
   int i;
   for (i= fg_current + 1; i < FG_MAXCONSOLE;i++)
      {
            if (fg_vconsoles[i]!=0)
               if (!fg_vconsoles[i]->ignore)
                {candidate = i;break;};
      };   
   if (candidate != -1) fg_setforeground(candidate);
};


void fg_setmykeyboard(int pid)
{
fg_processinfo *ptr = fg_getinfo(getprocessid());
if (ptr!=0)
    {
    ptr->keyboardfocus = pid;
    };
};

fg_processinfo *fg_getinfo(int pid)
{
int i;
for (i=0;i<FG_MAXCONSOLE; i++)
    {
        if (fg_vconsoles[i]!=0)
                {
                      if (fg_vconsoles[i]->pid == pid) return fg_vconsoles[i];
                };
    };
return 0;
};


fg_processinfo *fg_getmyinfo()
{
return fg_getinfo(getprocessid());
};

fg_processinfo *fg_register(DEX32_DDL_INFO *scr, int keyboard)
{
//first check if there is any slot for another virtual console
int i, slot = -1;
DWORD cpuflags;
fg_processinfo *new_vconsole;
for (i=0; i < FG_MAXCONSOLE; i++)
    {
        if (fg_vconsoles[i] == 0) {slot = i;break;};
    };

//oops! no more slots left, return with error
if (slot == -1) return -1; 
new_vconsole = (fg_processinfo*) malloc(sizeof(fg_processinfo));
memset(new_vconsole,0,sizeof(fg_processinfo));

/*Lock when entering a mutual exclusion section*/
dex32_stopints(&cpuflags);

new_vconsole->id = slot;
new_vconsole->size = sizeof(fg_processinfo);
new_vconsole->screen = scr;
new_vconsole->keyboardfocus = keyboard;
new_vconsole->pid = getprocessid();
fg_vconsoles[slot] = new_vconsole;

dex32_restoreints(cpuflags);

return new_vconsole;
};

int fg_exit()
{
DWORD cpuflags;
fg_processinfo *ptr;    

    dex32_stopints(&cpuflags);
    
    ptr = fg_getmyinfo();
    
    if (ptr!=0)
        {
            fg_vconsoles[ptr->id] = 0;
            free(ptr);
            fg_prev();
        };
        
    dex32_restoreints(cpuflags);
        
};

int fg_getkeyboardowner()
{
    if (fg_vconsoles[fg_current]!=0)
        return fg_vconsoles[fg_current]->keyboardfocus;
    else
        return 0; 
};

void fg_showmenu(int choice)
{
int i;
DWORD cpuflags;

fg_processinfo screens[FG_MAXCONSOLE];

dex32_stopints(&cpuflags);

for (i=0; i < FG_MAXCONSOLE; i++)
{
    if (fg_vconsoles[i])
        memcpy(&screens[i], fg_vconsoles[i], sizeof(fg_processinfo));
    else
        screens[i].id = -1;
};

dex32_restoreints(cpuflags);

for (i=0; i < FG_MAXCONSOLE; i++)
    {
      char pname[255];
      
       if (choice == i) textbackground(BLUE);
            else textbackground(BLACK);
                  
      if (screens[i].id!=-1)
      {
              PCB386 *process=ps_findprocess(screens[i].pid);
              if (process != -1)
              {
                  PCB386 *subprocess = ps_findprocess(screens[i].keyboardfocus);
                  strcpy(pname,process->name);
                  
                  if (screens[i].keyboardfocus != screens[i].pid 
                              && subprocess!=0)
                  sprintf(pname,"%s (%s)",process->name,subprocess->name);   
                  
                  if (screens[i].ignore)
                  textcolor(RED);
                     else
                  textcolor(WHITE);
                  
                  printf("[%2d] %-73s\n",i, pname);
                  textbackground(BLACK);
               }
                  else
               printf("[%2d] unknown\n",i);
       }
              else
       printf("[%2d] unused\n", i+1);
    };


};

void fg_set_state(int state)
{
static int fg_before;

if (fg_started)
  {
  fg_state = state;
  if (state) 
          fg_before=fg_setforeground(fg_myslot);
       else
          fg_setforeground(fg_before);
  };
 
};

#define KEY_UP  151
#define KEY_DOWN 152

void fg_updateinfo()
{
int refreshrate=100; //determines how long in milliseconds until the task manager refreshes its display
int loop;
int choice = 0;
 //Create a screen buffer that we can use   
 fg_out = Dex32CreateDDL();
 
 //register myself
 fg_fginfo = fg_register(fg_out,getprocessid());
 
 fg_myslot = fg_fginfo->id;
 
 //prevent F5 and F6 from setting this process as the foreground
 fg_fginfo->ignore = 1; 
 
 Dex32SetProcessDDL(fg_out,getprocessid());
 fg_started = 1;

 while(1)
    {
        if (fg_state)
        {
        dd_swaptomemory(fg_out);
        clrscr();
        textbackground(BLUE);
        textcolor(WHITE);
        printf("%-79s\n","Dex32- Virtual Console Management");
        textbackground(BLACK);
        
        //Display processes in memory
        fg_showmenu(choice);
        
        dd_swaptohardware(fg_out);
        if (kb_keypressed())
        {
           unsigned char c = getch();
           if (c-'0' >= 0 && c-'0' <= 9)
                {
                 int choice = c - '0';
                 if (fg_vconsoles[choice]!=0)
                 if (!fg_vconsoles[choice]->ignore)
                 fg_setforeground(choice);
                 
                }
           else
           if (c == KEY_UP)
                {
                 if (choice>0) choice -- ;
                }
           else 
           if (c == KEY_DOWN)
                {
                 if (choice<FG_MAXCONSOLE -1) choice++;
                }
           else
           if (c== '\n')
                {
                  if (fg_vconsoles[choice]!=0)
                  if (!fg_vconsoles[choice]->ignore)
                  fg_setforeground(choice);
                };      
        };
        };
    };
};

