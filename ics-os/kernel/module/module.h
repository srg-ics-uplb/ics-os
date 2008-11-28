/*
  Name: Extensible module loader
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 13/03/04 06:31
  Description: This module is responsible for interpreting the executable format
  of a program and then mapping it into memory so that it could be executed. Other
  tasks include loading libraries, dynamic linking etc. This module is called by
  the process dispatcher
  
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
#ifdef _MODULE_H_
#define _MODULE_H_

#define LOADERMAX 20
#define DEFAULT_STACKSIZE 0x100000
#define DEFAULT_HEAPSIZE 0x100000
#define MODULE_UNRESOLVED_IMPORT 0xFFFFABCD

#define PE_USEREXE 0
#define PE_USERDLL 1
#define PE_SYSEXE  2
#define PE_SYSDLL  3


typedef struct _lib_PCB
    {
      int size;
      int id;
      int lock;
      char name[255];     //name of the library
      DWORD version;
      void *exporttable;
      DWORD baseaddress;
      struct _lib_PCB *next;
      process_mem *memptr;
      void (*libentrypoint)(devmgr_generic *ptr);
      void (*libexit)();
    } lib_PCB;

int dex32_registerloader(void *funcptr);
int dex32_unregisterloader(void *funcptr);
void dex32_initloader();
int dex32_loader(char *name,char *image,char *loadaddress,int mode,char *p,char *workdir,PCB386 *parent);

#endif
