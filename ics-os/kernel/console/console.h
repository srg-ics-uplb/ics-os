/*
   ==========================================================================
   Console.c
   Author: Joseph Emmanuel Dayo
   Date updated:December 6, 2002
   Description: A kernel mode console that is used for debugging the kernel
   and testing new kernel features.
                
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
   ==========================================================================
*/
#ifndef _CONSOLE_H
#define _CONSOLE_H

#define CONSOLE_SIZE 80*25*2
#define CONSOLE_STACK_SIZE 0x2000
#define SORT_NAME 0
#define SORT_SIZE 1


int console_first = 0;
void console_main();
void getstring(char *buf,DEX32_DDL_INFO *dev);
void meminfo();
int delfile(char *fname);
int user_fork();
int user_execp(char *fname,DWORD mode,char *params);
int exec(char *fname,DWORD mode,char *params);
int user_exec(char *fname,DWORD mode,char *params);
int loadDLL(char *name,char *p);
void loadfile(char *s,int,int);
void loadlib(char *s);
int console_showfile(char *s,int wait);
DWORD alloc_console();
void console();
void prompt_parser(const char *promptstr,char *prompt);
int console_ls_sortsize(vfs_node *n1,vfs_node *n2);
int console_ls_sortname(vfs_node *n1,vfs_node *n2);
void console_ls(int style, int sortmethod);
int console_execute(const char *str);
int console_new();
void console_main();

#endif
