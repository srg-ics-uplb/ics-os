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



#define LOADERMAX 20
#define DEFAULT_STACKSIZE 0x100000
#define DEFAULT_HEAPSIZE 0x100000

#define MODULE_UNRESOLVED_IMPORT 0xFFFFABCD

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

lib_PCB *firstlibrary=0,*lastlibrary=0;
int lib_nextid=1;

#include "coff.c"
#include "pe_module.c"
#include "elf_module.c"
#include "b32.c"




int (*call_loader)(char *name,char *image,DWORD base,int mode,char *p,char *workdir,PCB386 *parent)=0;
int loader_total=0;
void *dex32_loaders[LOADERMAX];


int dex32_registerloader(void *funcptr)
  {
   if (loader_total>=LOADERMAX) return 0;
   dex32_loaders[loader_total]=funcptr;
   loader_total++;
   return 1;
  ;};

int dex32_unregisterloader(void *funcptr)
  {
   int i;
   for (i=0;i<loader_total;i++)
     {
        if (dex32_loaders[i]==funcptr)
          {
            dex32_loaders[i]=dex32_loaders[loader_total-1];
            loader_total--;
            return 1;
          };
     };
    return 0;
  };
  
void dex32_initloader()
  {
    dex32_registerloader(coff_loadusermodule); /*a.out type executables */	
    dex32_registerloader(pe_loadusermodule);   /*WIN32 PE executables   */
    dex32_registerloader(elf_loadmodule);      /*Linux elf executables  */	
    dex32_registerloader(b32_loadusermodule); /*DEX32 standard binary fomat*/    
  };

int dex32_loader(char *name,char *image,char *loadaddress,int mode,char *p,char *workdir,PCB386 *parent)
  {
     int i,ret;
     for (i=0;i<loader_total;i++)
       {
       call_loader=dex32_loaders[i];
       if (ret=call_loader(name,image,(DWORD)loadaddress,mode,p,workdir,parent)) return ret;
       };
     printf("dex32_loader: unidentified executable format.\n");  
   return 0;
  };



