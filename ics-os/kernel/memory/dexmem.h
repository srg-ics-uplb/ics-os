/*
  Name: dex low-level memory management library
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 02/03/04 18:06
  Description: This module handles everything that has to do
  with memory, except the high-level memory functions like malloc....
  
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


/*This constants define the selector values used by DEX,
  Unlike real-mode, protected mode uses selectors instead of segments. Selectors
  point to a table (the GDT) which contains information like the base of the segment
  and protection bits (See the an Intel 386/486/Pentium manuel for details on these*/
#define LINEAR_SEL 8
#define SYS_CODE_SEL 56
#define SYS_DATA_SEL 0x20
#define SYS_STACK_SEL 0x18
#define SYS_TSS   0x80
#define SCHED_TSS 0x78
#define USER_CODE 0x88+3
#define USER_DATA 0x90+3
#define USER_STACK 0x98+3
#define USER_TSS 0xA0+3
#define SYS_SCHED_SEL 0xA8
#define SYS_ERROR_TSS 0xB0
#define PF_TSS 0xB8
#define APM_CS32 0xC0
#define APM_CS16  0xC8
#define APM_DS 0xD0
#define DEX_SYSCALL 0xD8
#define KEYB_TSS    0xE0
#define MOUSE_TSS    0xE8


#define MACHINE_INTEL386

//******defines known page attributes********
#define PG_PRESENT 1
#define PG_WR 2
#define PG_USER 4
#define PG_WRITETHROUGH 8
#define PG_DIRTY  64
#define PG_DEMANDLOAD 0x200
#define PG_COPYWRITE 0x400


//this function returns the total available memory detected by startup.asm
DWORD memamount;

//this structure defines an entry in the gdt table
typedef struct __attribute__((packed)) _GDTentry
   {
      WORD limit;
      WORD lowaddr;
      BYTE lowaddr2;
      BYTE att1,att2;
      BYTE highaddr;
   } gdtentry;


//this tructure defines an 80386/486/Pentium call gate
typedef struct __attribute__((packed)) _CALLGATE
  {
    WORD lowoffset;
    WORD selector;
    BYTE attb1;
    BYTE attb2;
    WORD highoffset;
  } CALLGATE;

//this structure defines an entry in the IDT table
typedef struct _IDTentry
  {
    WORD lowphy ;  // the low word
    WORD selector; // the selelctor
	 BYTE reserved; // the reserved byte
	 BYTE attr;     // the attribute byte
    WORD highphy;  // the high word
  } idtentry;

DWORD totalgdtentries=10;

/*====================The DEX Memory Map==========================================*/

char *kbaseheap=(char*)0xC0000000;          //marks the location of the kernel heap
char *kmodeproc=(char*)0xD0000000,
     *kmodeproc_next=(char*)0xD0000000;     //marks the location of the kernel mode
                                            //process address space
                                            
char *lmodeproc=(char*)0xE0000000,          //marks the location of the shared library / driver 
     *lmodeproc_next= (char*)0xE0000000;    //address space
char *knext=          (char*)0xC0000000;    //marks the location of the top of the kernel heap

char *userstackloc=   (char*)0xB0000000;    //marks the location of the user stack
char *userheap=       (char*)0xA0000000;    //marks the base of the user heap
char *syscallstack=   (char*)0x90000000;    //marks the base of the system call stack
char *linux_userspace=(char*)0x80000000;    //marks the base where linux executables like to go
char *sharedmemloc=   (char*)0x70000000;    //marks the location of the user shared memory area
char *userspace=      (char*)0x00400000;    //marks the location of the user base address
DWORD *stackbase=    (DWORD*)0x00200000;    //marks the location of the stack pages
DWORD *kernelbase=   (DWORD*)0x00100000;    //marks the location of the kernel
idtentry *dex_idtbase=(idtentry*)0x2000;    //marks the location of the IDT
gdtentry *dex_gdtbase=(gdtentry*)0x1000;    //marks the location of the GDT
/*=================================================================================*/

/*Stores the total number of pages and memory respectively*/                
DWORD totalpages=0;
DWORD totalmemory=0;

//holds physical location of the kernel page directory
DWORD *pagedir1;


//prototypes for page management
extern void enablepaging();
extern void disablepaging();
extern void switchuserprocess(void);
extern inline void storeflags(DWORD *flags);
extern inline void restoreflags(DWORD flags);
extern void setpagedir(DWORD *dir);
inline void startints();
inline void stopints();

/*=================================Prototype definitions here==============================*/

WORD addgdt(DWORD base,DWORD limit,BYTE attb1,BYTE attb2);
void clearpagetable(DWORD *pagetable);
void *commit(DWORD virtualaddr,DWORD pages);
void *commitb(DWORD virtualaddr,int amt,DWORD *pagecount);
DWORD mem_detectmemory(mmap *grub_meminfo , int size );
void dex32copyblock(DWORD vdest,DWORD vsource,DWORD pages,DWORD *pagedir);
void *dex32_commitblock(DWORD virtualaddr,int amt,
    DWORD *pagecount,DWORD *pagedir,DWORD attb);
void *dex32_commit(DWORD virtualaddr,DWORD pages,DWORD *pagedir,DWORD pattb);
void dex32_copy_on_write(DWORD *directory);
void dex32_copy_pagedir(DWORD *destdir,DWORD *source);
void dex32_copy_pagedirU(DWORD *destdir,DWORD *source);
int dex32_copy_pg(DWORD *destdir, DWORD *source);
void dex32_freeuserpagetable(DWORD *pgd);
DWORD dex32_getfreepages();
void *dex32_reserveblock(DWORD virtualaddr,int amt,
    DWORD *pagecount,DWORD *pagedir,DWORD attb);
void *dex32_reserve(DWORD virtualaddr,DWORD pages,DWORD *pagedir,DWORD attb);
void *dex32_sbrk(unsigned int amt);
void freelinearloc(void *linearmemory,DWORD *pagedir);
void freemultiple(void *linearmemory,DWORD *pagedir,DWORD pages);
void freeuserheap(DWORD *pagedir);
DWORD getlinearloc(void *linearmemory,DWORD *pagedir);
DWORD getmultiple(void *linearmemory,DWORD *pagedir,DWORD pages);
DWORD getpagetablephys(DWORD vaddr,DWORD *pagedir);
DWORD getphys(DWORD vaddr,DWORD *pagedir);
DWORD getvirtaddress(DWORD physicaladdr);
DWORD getvirtaddress2(DWORD physicaladdr,DWORD hdl);
void maplineartophysical(unsigned int *pagedir,unsigned int linearaddr,
      unsigned int physical,unsigned int attribute);
int  maplineartophysical2(unsigned int *pagedir,unsigned int linearaddr,
      unsigned int physical,unsigned int attribute);      
DWORD xmaplineartophysical(const DWORD linearmemory,const DWORD physicalmemory,
   DWORD *pagedir,const DWORD attb);
void mem_init();
DWORD *mempop();
void mempush(DWORD mem);
DWORD obtainpage();
extern void refreshpages();
void setattb(WORD sel,BYTE attb1);
void setinterruptvector(DWORD index,idtentry *t,unsigned char attr,
     void (*handler)(int irq), WORD sel);
void dex32_setbase(WORD sel,DWORD addr);
void *sbrk(int amt);
void setcallgate(DWORD sel,DWORD funcsel,void *entry,BYTE params,BYTE access);
void setgdt(WORD sel,DWORD base,DWORD limit,BYTE attb1,BYTE attb2);
void setgdtentry(DWORD index,void *base,gdtentry *t,DWORD limit,
           BYTE attb1,BYTE attb2);

void mem_interpretmemory(mmap *map,int size);
void dex32_stopints(DWORD *flags);
void dex32_restoreints(DWORD flags);
void setpageattb(DWORD *pagedir,DWORD vaddr,DWORD attb);
void *dex32_setpageattb(DWORD virtualaddr,DWORD pages,DWORD *pagedir,DWORD pattb);
void *dex32_setpageattbblock(DWORD virtualaddr,int amt,DWORD *pagecount,DWORD *pagedir,DWORD attb);
