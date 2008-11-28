/*
  Name: DEX device manager
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 10/02/04 20:50
  Description: The device manager is the primary module for handling the device
  in the system. A device in DEX may also include the scheduler, Virtual Memory Manager
  and other operating system services.
  
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

#ifndef DEVMGR_HEADER
#define DEVMGR_HEADER

#include "../dextypes.h"
#include "../process/sync.h"
#include "../process/process.h"

//The maximum number of filesystems that could be supported
#define MAXDEVICES 255

//The types of devices

//Unknown device
#define DEVMGR_UNKNOWN 0
//filesystem drivers
#define DEVMGR_FS  1

//generic device driver
#define DEVMGR_DEV     2

//generic extension module
#define DEVMGR_EXT     3

//block device driver
#define DEVMGR_BLOCK   4

//device manager
#define DEVMGR_SERVICE 5

//standard library interface
#define DEVMGR_STDLIB 6

//standard character device
#define DEVMGR_CHAR 7

//A port manager
#define DEVMGR_PORTMGR 8

//The io manager interface
#define DEVMGR_IOMGR 9

//The memory manager interface
#define DEVMGR_MEM 10

//The process manager interface
#define DEVMGR_PROCESSMGR 11

//The graphics library interface
#define DEVMGR_GRAPHICS 12

//A PCI bus driver
#define DEVMGR_PCI 13

//process scheduler interface
#define DEVMGR_SCHEDULER_EXTENSION 0x1000

//kernel heap memory management
#define DEVMGR_MALLOC_EXTENSION 0x1001

//The Virtual Memory Manager Interface
#define DEVMGR_VMM_EXTENSION 0x1002

#define DEVMGR_MESSAGESTR 1

//Types of information
#define DEVMGR_BLOCK_INFO 0x1000

//Types of IOCTL FUNCTIONS
#define DEVMGR_IOCTL_GETINFO 0
#define DEVMGR_IOCTL_READ  2
#define DEVMGR_IOCTL_WRITE 3

/* devmgr_generic is the basic header of all devices and extension modules in the
  DEX operating system */
typedef struct _devmgr_generic {
int size;           //size of this structure
int id;             //An ID assigned by the Device manager
int type;           //Identifies what kind of device this is
int source;         //holds the module ID of the library or 0 if it is from the kernel
char name[20];      //The name of the device
char description[256]; //a short description of the device
int lock;           //determines if this device is already being used
int (*sendmessage)(int type, int message); //used by the device to receive messages
} devmgr_generic;

/*Devmgr_extension_header is the basic header of all extension modules aside
  from the devmgr_generic header which is also included.*/
typedef struct _devmgr_extension_header {
int busywait,bridgeraised;           //used for controlling access to the extension module
int (*remove)();                     //called when this module is to be removed 
int (*attach)(devmgr_generic *cur);  //called when this module is to be attached
int (*pre_remove)();                 //same as remove, only that interrupts are available
                                     //at the time this is called.   
} devmgr_extension_header;


/*==== The definitions of the various interfaces DEX supports follows. You may add
       more types of interfaces here if you wish, just remeber to modify devmgr_identify ==*/

typedef struct _devmgr_iomgr {
devmgr_generic hdr;
DWORD (*init)();
int   (*complete)(DWORD handle);
void  (*close)(DWORD handle);
DWORD (*request)(int,int,DWORD,DWORD,void*);
} devmgr_iomgr;

typedef struct _devmgr_mem {
devmgr_generic hdr;
void *(*sbrk)(int amt);
void *(*mem_map)(unsigned int *pagedir,unsigned int vitualaddr,
                 unsigned int physicaladdr,unsigned int attribute);
void *(*commit)(DWORD virtualaddr,DWORD pages);
void (*freemultiple)(void *virtualaddr,DWORD *pagedir,DWORD pages);
} devmgr_mem;


typedef struct _devmgr_block_info {
devmgr_generic hdr;
int blocksize;
int maxblocks;
} devmgr_block_info;

typedef struct _devmgr_status {
int size;           //size of this structure
int locked;
} devmgr_status;

typedef struct _devmgr_pci {
devmgr_generic hdr;
int   (*pci_writeconfigdword)(char bus, char devfunc, WORD register, DWORD data);
int   (*pci_writeconfigword)(char bus, char devfunc, WORD register, WORD data);
int   (*pci_writeconfigbyte)(char bus, char devfunc, WORD register, BYTE data);
DWORD (*pci_readconfigdword)(char bus, char devfunc, WORD register);
WORD  (*pci_readconfigword)(char bus, char devfunc, WORD register);
BYTE  (*pci_readconfigbyte)(char bus, char devfunc, WORD register);
int   (*pci_finddevice)(WORD deviceid,WORD vendorid, 
           WORD index,char *busnumber, WORD *devnumber);
int   (*pci_findclass)(WORD classcode, WORD index,char 
           *busnumber, WORD *devnumber);
int (*pci_init)();
} devmgr_pci;

/*For block devices like Hard Disks CD-ROM drives, floppies, Ram Disks etc*/
typedef struct _devmgr_block_desc {
devmgr_generic hdr;

int (*init_device)();
int (*invalidate_cache)();
int (*getcache)(char *buf,DWORD sectornumber,DWORD numblocks);
int (*read_block)(int block,char *blockbuff, DWORD numblocks);
int (*write_block)(int block,char *blockbuff,DWORD numblocks);
int (*total_blocks) ();
int (*flush_device)();
int (*get_block_size)();
int (*putcache)(char *buf,DWORD sectornumber,DWORD numblocks);
} devmgr_block_desc;

/*The Interface to the DEX Process Manager*/
typedef struct _devmgr_processmgr {
devmgr_generic hdr;
int    (*getprocessid)(); //obtain the pid of the current process
DWORD  (*getparentid)();  //obtain the pid of the parent process
} devmgr_processmgr;

//virtual memory manager
typedef struct _devmgr_vmm {
devmgr_generic hdr;
devmgr_extension_header exthdr;
DWORD (*vmm_getpages)(); /* called by the memory manager when there is little or 
                            no more physical frames left*/
int   (*vmm_pagein)(DWORD memloc);//called during a page fault and the page is marked as paged-out
void  (*vmm_free)(DWORD memloc); //gets called by the process manager when a page should be freed
} devmgr_vmm;

typedef struct _devmgr_scheduler_extension {
devmgr_generic hdr;
devmgr_extension_header exthdr;
//extensible functions
PCB386 *(*ps_gethead)();
int     (*ps_dequeue)(PCB386 *);
void    (*ps_enqueue)(PCB386 *process);
PCB386 *(*scheduler)(PCB386 *lastprocess);
int    (*ps_listprocess)(PCB386 *process_buf,DWORD size_per_item, int items);
PCB386 *(*ps_findprocess)(int pid);
} devmgr_scheduler_extension;


typedef struct _devmgr_malloc_extension {
    devmgr_generic hdr;
    devmgr_extension_header exthdr;
    //extensible functions
    void *(*malloc)(int nbytes);
    void (*free)(void *cp);
    void *(*realloc)(void *cp, int nbytes);
} devmgr_malloc_extension;

//defines a stdout interface
typedef struct _devmgr_stdlib {
    devmgr_generic hdr;
    int  (*printf)(const char *fmt, ...);
    void *(*malloc)(int);
    void (*free)(void*);
    void *(*realloc)(void *,int);
} devmgr_stdlib;

//defines a VGA graphics driver interface
typedef struct _devmgr_graphics {
    devmgr_generic hdr;
    int  (*set_graphics_mode)(int mode);
    int  (*set_text_mode)(int mode);
    int  (*write_pixel)(DWORD x,DWORD y,DWORD color);
    int  (*read_palette)(char *r,char *g,char *b,char index);
    void (*write_palette)(char r,char g,char b,char index);
} devmgr_graphics;

//defines a device manager interface
typedef struct _devmgr_interface {
    devmgr_generic hdr;
    int  (*devmgr_register)(devmgr_generic *);
    char *(*devmgr_identify)(int type,char *buf);
    void (*devmgr_init)();
    int  (*devmgr_flushblocks)();
    int  (*devmgr_finddevice)(const char *name);
    int  (*devmgr_copyinterface)(const char *name,devmgr_generic *interface);
    devmgr_generic (*devmgr_getdevice)(int deviceid);
    void (*devmgr_disableints)();
    void (*devmgr_enableints)();
    int  (*extension_override)(devmgr_generic *ext, devmgr_generic **prev);
} devmgr_interface;

//defines a filesystem interface
typedef struct _devmgr_fs_desc {
    devmgr_generic hdr;
    //For filesystem drivers only
    int (*identify)(int device);				    //OPTIONAL- Used to detect if <device> supports this FS
    int (*mountroot)(vfs_node*,int device);    //called by the VFS when mounting a device
    int (*rewritefile)(vfs_node*,int device);
    int (*readfile)(vfs_node*,char*,int,int,int device);
    int (*chattb)(vfs_node*,const int,int device);
    int (*getsectorsize)(vfs_node*,int device);
    int (*deletefile)(vfs_node*,int device);
    int (*addsectors)(vfs_node*,int,int device);
    int (*writefile)(vfs_node*,char*,int,int,int device);
    int (*createfile)(vfs_node*,int device);
    int (*getbytesperblock)(int device);
    int (*validate_filename)(const char *filename);
    int (*mountdirectory)(vfs_node*,int device);
    int (*unmount)(vfs_node*);
} devmgr_fs_desc;

typedef struct _devmgr_hwportmgr {
    devmgr_generic hdr;
    int  (*ports_close)(DWORD devid, DWORD portnum, BYTE attb);
    BYTE (*ports_getstatus)(DWORD portnum);
    int  (*ports_openport)(DWORD devid, DWORD portnum,BYTE attb);
    int  (*ports_read)(DWORD devid, DWORD portnum,WORD data,int datasize);
    int  (*ports_setattb)(DWORD portnum,BYTE attb);
    int  (*ports_write)(DWORD devid, DWORD portnum,WORD data,int datasize);  
} devmgr_hwportmgr;

typedef struct _devmgr_char_desc {
    devmgr_generic hdr;
    int   (*init_device)();            //initializes the device
    int   (*ready_put)();              //1 if ready, 0 if not ready for output
    int   (*ready_get)();              //1 if ready, 0 if not ready for input
    int   (*get_char)();               //obtains a character from the device, if it is ready
    int   (*put_char)(int c);          //puts a character into the device, if it is ready
    int   (*set_callback_handler)(int event, void *fxnaddr); //assigns a callback fxn for an event
    void *(*get_callback_handler)(int event);//returns the address of the fxn for the event
} devmgr_char_desc;



extern devmgr_generic **devmgr_devlist;         //points to the list of devices
extern devmgr_status *devmgr_statuslist;
extern sync_sharedvar devmgr_busy;


/*---------------------------------Function Prototypes Here----------------------------------*/
int  devmgr_copyinterface(const char *name,devmgr_generic *interface);
devmgr_generic *devmgr_getdevice(int deviceid);
devmgr_generic *devmgr_getdevicebyname(const char *name);
int  devmgr_finddevice(const char *name);
int  devmgr_flushblocks();
int  devmgr_getcontext();
int  devmgr_getfunction();
int  devmgr_getlock(int devicehandle);
char *devmgr_getname(int deviceid);
char *devmgr_identify(int type,char *buf);
void devmgr_init();
int  devmgr_register(devmgr_generic *);
int  devmgr_removedevice(int deviceid);
int  devmgr_sendmessage(int deviceid,int type,DWORD message);
void devmgr_setcontext(int deviceid);
void devmgr_setfunction(int fxn);
int  devmgr_setlock(int devicehandle,int val);
void *devmgr_tweak_by_ID(int deviceid, int function_index, void *new_function);
void *devmgr_tweak_by_addr(devmgr_generic *mydev, void **target_function, void *new_function);
void devmgr_showdevices();

#endif
