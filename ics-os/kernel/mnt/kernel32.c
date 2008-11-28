/*
  Name: DEX-OS 1.0 Beta Kernel Main file
  Copyright: 
  Author: Joseph Emmanuel Dayo
  Date: 13/03/04 06:20
  Description: This is the kernel main file that gets called after startup.asm.
  
   
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

#define NULL 0
#define DEBUGX
#define USE_CONSOLEDDL


/*some defines that are used for debugging purposes*/

#define DEBUG_FLUSHMGR
//#define DEBUG_COFF
//#define DDL_DEBUG
//#define DEBUG_FORK
#define FULLSCREENERROR
//#define DEBUG_KSBRK
//#define DEBUG_FAT12
#define DEBUG_STARTUP
#define DEBUG_EXTENSION
//#define DEBUG_USER_PROCESS
//#define MODULE_DEBUG
//#define DEBUG_PEMODULE
//#define DEBUG_VFSREAD
//#define DEBUG_MEMORY
//#define USE_DIRECTFLOPPY
//#define DEBUG_VFS
//#define DEBUG_IOREADWRITE
//#define DEBUG_IOREADWRITE2
//#define WRITE_DEBUG2
//#define WRITE_DEBUG
//#define DEBUG_READ
//#define DEBUG_BRIDGE



//timer set to switch to new task 200 times per second. (see time.h or time.c)
int context_switch_rate=100; 
char *scr_debug = (char*)0xb8000;

int op_success;

//points to the location of the multiboot header defined in startup.asm
extern int multiboothdr; 
extern void textcolor(unsigned char c);


//order is important for some include files, DO NOT CHANGE!
#include <stdarg.h>
#include <limits.h>

#include "dextypes.h"
#include "process/sync.h"
#include "stdlib/time.h"
#include "stdlib/dexstdlib.h"
#include "startup/multiboot.h"
#include "memory/dexmem.h"
#include "console/dex_DDL.h"
#include "vfs/vfs_core.h"
#include "process/process.h"
#include "process/pdispatch.h"
#include "devmgr/dex32_devmgr.h"
#include "devmgr/devmgr_error.h"
#include "console/dexio.h"
#include "hardware/keyboard/keyboard.h"
#include "hardware/hardware.h"
#include "hardware/chips/ports.c"
#include "hardware/vga/dexvga.c"
#include "hardware/floppy/floppy.h"
#include "hardware/ATA/ataio.h"
#include "hardware/exceptions.h"
#include "hardware/chips/speaker.h"
#include "dexapi/dex32API.h"
#include "filesystem/fat12.h"
#include "filesystem/iso9660.h"
#include "filesystem/devfs.h"
#include "process/event.h"
#include "devmgr/extensions.h"
#include "process/environment.h"
#include "console/foreground.h"
#include "console/console.h"
#include "stdlib/stdlib.h"
#include "devmgr/bridges.h"
#include "process/scheduler.h"
#include "console/script.h"
#include "vfs/vfs_aux.h"
#include "memory/kheap.h"
#include "iomgr/iosched.h"
typedef struct _kernel_sysinfo {
int boot_device;
int part[3];
} kernel_sysinfo;

kernel_sysinfo kernel_systeminfo;

//This stores the current virtual console the kernel will use
DEX32_DDL_INFO *consoleDDL;
char *dex32_versionstring="DEX Extensible Operating System, project \"Chameleon\" \
\nVersion 1.02 build April 9 2004\n \
Copyright (C) 2004  Joseph Emmanuel DL Dayo\n \
Developed as a requirement for CMSC 190 at the Institute of Computer Science\n \
University of the Philippines, Los Baños.\n\n \
This program is free software; you can redistribute it and/or modify\n \
it under the terms of the GNU General Public License\n";

void dex_kernel32();

/*I know there are some disadvantages to directly including files
  in the source code instead of using object files, but it simplifies
  compilation without the use of a makefile*/
#include "console/dex_DDL.c"
#include "hardware/dexapm.c"
#include "hardware/chips/irqhandlers.c"
#include "memory/dlmalloc.c"
#include "memory/bsdmallo.c"
#include "hardware/floppy/floppy.c"
#include "vfs/vfs_core.c"
#include "module/module.c"
#include "process/pdispatch.c"
#include "console/console.c"
#include "stdlib/time.c"
#include "console/dexio.c"
#include "stdlib/stdlib.c"
#include "process/dex_taskmgr.c"
#include "hardware/keyboard/keyboard.c"
#include "hardware/pcibus/dexpci.c"
#include "hardware/exceptions.c"
#include "hardware/hardware.c"
#include "hardware/chips/speaker.c"
#include "devmgr/dex32_devmgr.c"
#include "devmgr/extension.c"
#include "process/environment.c"
#include "stdlib/qsort.c"
#include "console/foreground.c"
#include "devmgr/bridges.c"
#include "process/sync.c"
#include "console/script.c"
#include "process/process.c"
#include "dexapi/dex32API.c"
#include "hardware/ATA/ide.c"
#include "vfs/vfs_aux.c"
#include "memory/kheap.c"
#include "memory/dexmem.c"
#include "memory/dexmalloc.c"
#include "vmm/vmm.c"

void dex32_startup(); //start up the operating system
extern startup();

fg_processinfo *fg_kernel = 0;


/*the start of the main kernel-- The task here is to setup the memory
  so that we could use it, we also enable some devices like the keyboard 
  and the floppy disk etc.
  
  Assumptions:
  DEX assumes that at this point the following should be true:
  
  * Protected Mode is enabled
  * paging is disabled
  * interrupts are disabled
  * The CS,DS,SS,ESP must already be set up, meaning that the GDT should already be present
  
  ORDER is important when starting up the kernel modules!!*/

multiboot_header *mbhdr = 0;

void main()
{

    char temp[255];
    
    /*obtain the multiboot information structure from GRUB which contains info about memory
      and the device that booted this kernel*/
    mbhdr =(multiboot_header*)multiboothdr;
    
    /* Enable the keyboard IRQ,Timer IRQ and the Floppy Disk IRQ.As more devices that uses IRQs get
       supported, we should OR more of them here*/
    program8259(IRQ_TIMER | IRQ_KEYBOARD | IRQ_FDC); 

    //sets up the default interrupt handlers, like the PF handler,GPF handler
    setdefaulthandlers();   
    
    /*and some device handlers like the keyboard handler
      initializes the keyboard*/
    installkeyboard(); 

     //obtain the device which booted this operating system         
     kernel_systeminfo.boot_device = mbhdr->boot_device >> 24;
     kernel_systeminfo.part[0] =    (mbhdr->boot_device >> 16) & 0xFF;
     kernel_systeminfo.part[1] =    (mbhdr->boot_device >> 8) & 0xFF;
     kernel_systeminfo.part[2] =    (mbhdr->boot_device & 0xFF);
        
     //obtain information about the memory configuration
     memory_map = mbhdr->mmap_addr;
     map_length = mbhdr->mmap_length;
        
     /*
      DEX stores the free physical pages as a stack of free pages, therefore
      when a physical page of memory is needed, DEX just pops it off the stack.
      If DEX recovers used memory, it is pushed to the stack.
      The createstack() function creates the physical pages stack.
      See dexmem.c for details*/
    
    memamount = mem_detectmemory(memory_map, map_length);

    
    /*The mem_init() function first sets up the page table/directories which
      is used by the MMU of the CPU to map vitual memory locations to physical 
      memory locations. Basically the first 3MB of physical memory is mapped
      one-to-one (meaning virtual memory location = physical memory location.
      Finally it assigns the the location of the page directory to the CR3
      register and then enables paging.
      
      NOtE: DEX uses the flat memory model and all segment registers used by
      DEX has a base equal to zero*/
    mem_init(); 
    
    /*The default values of the current_process variable, which is the kernel
      PCB*/
    current_process = &sPCB;

    //Program the Timer to context switch n times a second	
    dex32_set_timer(context_switch_rate);

    //initialize the bridge manager, see bridges.c for details
    bridges_init();
    
    //initialize the virtual console manager
    fg_init();
    
    //Create a virtual console that the kernel will send its output to
    consoleDDL=Dex32CreateDDL();
    fg_kernel = fg_register(consoleDDL,0);
    fg_setforeground(fg_kernel);
    
    /* Preliminary initializaation complete, start up the operating system*/
    dex32_startup(); 
};


void dex32_startup()
{

    
    /*At this point, memory accesses should already be safe, and
      until the scheduler starts, the interrupts must be disabled*/

    //Display some output for introductory purposes :)
    clrscr();
    textbackground(BLUE);
    textcolor(YELLOW);
    printf("DEX");
    textcolor(WHITE);
    printf("%-76s\n"," Extensible Operating System v.1.02 Beta project \"Chameleon\"");
    textcolor(WHITE);
    textbackground(BLACK);
    printf("BUILD April 9 2004\n");
    textbackground(BLACK);
    printf("This program is free software; you can redistribute it and/or modify\n");
    printf("it under the terms of the GNU General Public License\n");
    printf("======================================================================\n");
    printf("dex32_startup(): Welcome! starting up the DEX Operating System.\n");

    /*show parameter information sent by the multiboot compliant bootloader.*/
    printf("dex32_startup(): Bootloader name : %s\n", mbhdr->boot_loader_name);
    printf("dex32_startup(): Memory size: %d KB\n",memamount/1024);

    //Initialize the extension manager
    printf("dex32_startup(): Initializing the extension manager..\n");
    extension_init();


    //initialize the device manager
    printf("dex32_startup(): Initializing the device manager\n");
    devmgr_init();


    printf("dex32_startup(): Registering the memory manager and the memory allocator\n");

    //register the memory manager
    mem_register();

    //register the different memory allocators
    bsdmalloc_init();       //BSD malloc
    dlmalloc_init();        //Doug Lea's malloc
    dexmalloc_init();       //Joseph Dayo's (*poor*) first fit malloc function
    
    /* initialize the malloc server, place the device name of the malloc
       function you wish to use as the paramater*/
    alloc_init("dl_malloc"); 
    
    //register the hardware ports manager
    printf("dex32_startup(): Initializing ports\n");
    ports_init();

    //initialize the DEX API module
    api_init();

    printf("dex32_startup(): Initializing the process manager..\n");
    //Initialize the process manager
    process_init();

#ifdef DEBUG_STARTUP
    printf("dex32_startup(): process manager initialized.\n");
#endif   

   
    //process manager is ready, pass execution to the taskswitcher
    taskswitcher();

    //============ we should not reach this point at all =================
    while (1);
};

#define STARTUP_DELAY 400

/*This function is the first function that is called by the taskswitcher
  incidentally it is also the first process that gets run*/
void dex_kernel32()
{
    char temp[255],spk;
    int consolepid,i,baremode = 0;
    int delay_val =  STARTUP_DELAY / 80;
    devmgr_block_desc *myblock;
    dex32_datetime date;
    //At this point, the kernel has fininshed setting up memory and the process scheduler.
    //More importantly, interrupts are already operational, which means we can now set up
    //devices that require IRQs like the floppy disk driver 
    textcolor(GREEN);
    printf("   ------- Welcome! DEX operating system kernel initialized -------\n");
    printf("   press <spacebar> to skip startup script file ...\n");
    textcolor(WHITE);
      
    //initialize the keyboard device driver
    init_keyboard();
    
    //add some hotkeys to the keyboard
    kb_addhotkey(KEY_F6+CTRL_ALT, 0xFF, fg_next);
    kb_addhotkey(KEY_F5+CTRL_ALT, 0xFF, fg_prev);
    kb_addhotkey('\t', KBD_META_ALT, fg_toggle);
    
    keyboardflush();
    
    /*Now that the timer is active we can now use time based functions.
      Delay for two seconds in order to see previous messages */  
    textbackground(WHITE);
    for (i=0 ;i < 80; i++)
      {
          printf(" ");  
          if (kb_ready())
             if (getch() ==' ') {baremode = 1;break;};        
          delay( delay_val );
      };
    textbackground(BLACK);
      
    printf("\n");  

    //obtain CPU information using the CPUID instruction
    hardware_getcpuinfo(&hardware_mycpu);
    hardware_printinfo(&hardware_mycpu);

    getdatetime(&date);
    getmonthname(date.month,temp);

    //Install the built-in floppy disk driver
    floppy_install("floppy");
    
    /*Install the IDE, ATA-2/4 compliant driver in order to be able to
      use CD-ROMS and harddisks. This will also create logical drives from
      the partition tables if needed.*/
    ide_init();

    /*Install the VGA driver*/
    vga_init();
    
    //initialize the I/O manager
    iomgr_init();

    myblock = (devmgr_block_desc*)devmgr_devlist[floppy_deviceid];
    myblock->init_device();

    //initialize the pci device manager, which does not seem to work : )
    //init_pci();

    printf("dex32_startup(): Initializing the Virtual File System...\n");

    //initialize the file tables (Initialize the VFS)
    vfs_init();


    current_process->workdir= vfs_root;
    
    printf("dex32_startup(): Initializng the task manager..\n");
    //Initialize the task manager - a module program that monitors processes
    //for the user's convenience
    tm_pid=createkthread((void*)dex32_tm_updateinfo,"dex32_taskmanager",3500);


    //create the IO manager thread which handles all I/O to and from
    //block devices like the hard disk, floppy, CD-ROM etc. see iosched.c
    createkthread((void*)iomgr_diskmgr,"iomgr_diskmgr",200000);

   
    //Install a null block device
    devfs_initnull();
    
    //install and initialize the Device Filesystem driver
    devfs_init();
    
    //install and initialize the fat12 filesystem driver
    fat_register("fat");
    
    //initialize the CDFS (ISO9660/Joliet) filesystem
    iso9660_init();

    printf("Mounting boot device...\n");
    
    //mount the floppy disk drive
    vfs_mount_device("fat","floppy","boot");

    printf("dex32_startup(): Initializing first module loader(s) [EXE][COFF][ELF][DEX B32]..\n");

    //setup the initial executable loaders (So we could run .EXEs,.b32,coff and elfs)
    dex32_initloader();

    /*Supposed to initialize the Advanced Power Management Interface
      so that I could do a "software" shutdown **IN PROGRESS** */
    dex32apm_init();

    //Initialize the PCI bus driver
    init_pci();

    printf("dex32_startup(): Running foreground manager thread..\n");
    
    //create the foreground manager
    fg_pid = createkthread((void*)fg_updateinfo,"fg_manager",20000);
    
    if (baremode) console_first++;
    printf("dex32_startup(): Running console thread..\n");
    
    //Create a new console instance
    consolepid = console_new();


    /*beep the computer just in case a screen problem occured, at least
      we know it reaches this part*/
    spk=inportb(0x61);
    spk=spk|3;
    outportb(0x61,spk);
    delay(1);
    spk=inportb(0x61);
    spk=spk&252;
    outportb(0x61,spk);


    Dex32SetProcessDDL(consoleDDL, getprocessid());
    
	 
    /*Run the process dispatcher.
      The process dispatcher is responsible for running new modules/process.
      It is the only one that could disable paging without crashing the system since
      its stack, data and code segments are located in virtual memory that is at the
      same location as the physical memory
      see pdispatch.c/pdispatch.h for details*/
    process_dispatcher();
    ;
};

void end_func()
{
};
