/*
  Name: DEX32 Process Management Module
  Copyright: 
  Author: Joseph Emmanuel Dayo
  Date: 09/11/03 04:11
  Description: Provides functions for process management and task switching
  
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

#ifndef _PROCESS_H
#define _PROCESS_H

#include "../dextypes.h"
#include "../vfs/vfs_core.h"
#include "../console/dex_DDL.h"
                                
#define ACCESS_SYS 0
#define ACCESS_USER 1
#define ACCESS_DRIVER 2

#define MAX_MESSAGE 10

//approximately 2 seconds
#define TERM_TIME 400

//SEMAPHORE SIGNALS dex uses for managing applications
#define SIG_RUNNING 1

//defines the possible return values of send and get message
#define IPCSTAT_FULL
#define IPCSTAT_ERROR
#define IPCSTAT_OK

/*defines the OS defined IPC messages 0x00000000 - 0x0FFFFFFF
  the user programs are free to define their own messages from
  0x10000000-0xFFFFFFFF*/

//tells the process that it should terminate itself :)
#define SIG_TERM 0x1
//tells the process that it should terminate itself within TERM_TIME or
//a force kill will result
#define SIG_KILL 0x2

#define MES_SHUTDOWN 0x4

//Constants for the process status and sttributes
#define PS_ATTB_LOCKED      1
#define PS_ATTB_UNLOADABLE  2
#define PS_ATTB_BLOCKED     4
#define PS_ATTB_THREAD      8

//defines the PIDs of the SCHEDULER and the scheduler
#define SYSPID_KERNEL   0
#define SYSPID_SCHED    1

//defines the location of the stacks used by some of the kernel modules
#define SCHED_STACK_LOC      0x1AFFF0
#define DISPATCHER_STACK_LOC 0x1FFFF0
#define PAGEFAULT_STACK_LOC  0x1DFFF0

//defines the stack size for system calls of user process
#define SYSCALL_STACK 0xFFFF

/*process_mem is used to describe the memory space that is used by the process
  this is primarily used when first allocating space for a process in system
  memory. Dex32 uses the information here to reclaim any memory ued by the application
  when it terminates*/
typedef struct _process_mem {
    DWORD vaddr,pages;
    struct _process_mem *next;
} process_mem;


typedef struct _DEXPCB {
   DWORD processid;
   char name[256];      // the name of the process
   DWORD accesslevel;   //the security bits for this process
   DWORD priority;      //the process priority
   DWORD locked,unloadable;
   process_mem *meminfo;
   } DEXPCB;


//the process control block for the Intel 386
typedef struct __attribute__((packed)) _fsaveregs {
    //the first entries contain an exact copy of the 386 TSS
    DWORD TSSprev ;//back link to the previous TSS
    DWORD ESP0,SS0;
    DWORD ESP1,SS1;
    DWORD ESP2,SS2;
    DWORD CR3;
    DWORD EIP; //the instruction pointer
    DWORD EFLAGS; //the flags register
    DWORD EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI;
    //the 16 bit registers
    WORD  ES,RR0;
    WORD  CS,RR1;
    WORD  SS,RR2;
    WORD  DS,RR3;
    WORD  FS,RR4;
    WORD  GS,RR5;
    WORD  LDT,RR6;
    WORD  RR7,IO_MAP_BASE;
 } saveregs;


/* Defines a semaphore or a shared variable used primarily for
   snychronization among processes*/
typedef struct _SEMAPHORE {
DWORD handle; //a system generated number that is used to identify this semaphore
DWORD owner;  //defines the owner of this semaphore
DWORD attb;   //defines the permission bits of this semaphore
DWORD data;   //defines the data that is to be shard among processes
struct _SEMAPHORE *next; //points to the next semaphore
struct _SEMAPHORE *prev; //points to the previous semaphore
} semaphore;


/*Defines the structure for a message in a message passing
  interprocess communication*/
typedef struct _IPC_message {
DWORD sender,receiver;
DWORD message,data;
} IPC_message;

/*Part of the PCB386 struct and is used for holding the registers of the
 FPU during context switching*/
typedef struct _FPUregs {
DWORD fpu[27];
} FPUregs;

/*The primary data structure used for describing a process or thread,
  the first element of this data structure holds TSS data*/
typedef struct _PCB386 {
    /*regs must be at the beginning of the PCB since the 
      TSS directly points to the beginning of this structure*/
    saveregs regs;      /* TSS data, also for placing initial values for EAX,EBX etc.
                        A very hardware specific data structure for the Intel x86 family*/
    FPUregs regs2;      //stores the FPU registers, for Processors with FPU's
    DWORD size,version; //the size of the PCB386 structure is placed here
                        //this is for extensibility purposes
    DWORD *pagedirloc;  /*location of the processes active pagedirectory
                          NULL if page directory is the same as kernel's*/
    DWORD processid;    //a process ID that is generated by the system
    DWORD owner;        /*The owner refers to the pid of the process that spawned
                        this thread/process*/
    char name[256];     // the name of the process
    vfs_node *workdir;  //points to the vfs_node of the working directory 
    DWORD accesslevel;  //the security bits for this process
    DWORD priority;     //the process priority
    DWORD status;       //A flag for describing the status of the process
    DWORD waiting;      //Set to the amount of time for a sleeping process 
    DWORD childwait;    /*used by dex32_wait to check if a child has terminated
                        or not, incremented by 1 when a child process is spawned*/

    DWORD syscallsize;  //stores the size of the system call stack
    DWORD lastcputime, totalcputime; /*the taskswitcher increments this value every time
                                       the process takes control of the CPU*/
    DWORD arrivaltime;   //holds the time when this process arrived
    
    DWORD semhandle;    //holds the handle of the process semaphore
    
    process_mem *meminfo; /*points to a data structure containing the memory locatons taken up by
			            this process so that the process manager could clean this up easily.*/	

    void *stackptr,*stackptr0; //user and system bottom of stack pointers
    char *parameters;   //stores the parameters passed to this process
    char *imagesource;  //records the full path of the source file
    char *knext;        //holds the location of the programs' break (Or the end of the heap)
    char *environment;  //holds the location of the environment variables
    
    void (*putc)(char *c); //points the output function to be used (User mode apps only)
    char (*getc)();        //points to the input function to be used (User mode apps only)
    
    /*---------------------------------Process I/O data-------------------------------------*/
    DEX32_DDL_INFO *outdev;  //points to the output device descriptor
    void *stdout;            //For pipes *NOT YET IMPLEMENTED*
    void *stdin;             //For pipes *NOT YET IMPLEMENTED*
    int context,function;    /*used by device drivers to determine context info.
                               Reserved on USER mode programs.*/
    
    int meshead,mestotal,curmes;
    int lasterror; //used by API calls to set the last error on a per thread basis
    int misc;      //used for troublesome stdlib functions like strtok  

    struct _PCB386 *next;   //Points to the next PCB
    struct _PCB386 *before; //Points to the previous PCB
    
    IPC_message mesq[MAX_MESSAGE]; //pointer to the process message queue
    DWORD (*dex32_signal)(DWORD,DWORD);//the dex32 general signal handler
    
    //Debugging data
    DWORD cursyscall[2],op_success; //gives information about the last systems calls made    
    void *signaltable;              //Reserved for the signal table *NOT YET IMPLEMENTED*
    } PCB386;

typedef struct _TSSREG {
   WORD limit;
   WORD lowbase;
   BYTE high1;
   BYTE attb1;
   BYTE attb2;
   BYTE high2;
   } TSSREG;


extern semaphore *semaphore_head;

/* The next process to be created will use this process ID
   pids 1-0x8A is reserved, pid 0 is the kernel pid*/
extern DWORD nextprocessid;
    
//used for the busy waiting loops of the process manager.    
extern sync_sharedvar processmgr_busy;

extern int totalprocesses;

//global vriables used for triggering taskswitcher events.
extern DWORD sigpriority; //set this to the process ID of the process that requires immediate attention
extern DWORD sigterm;     //set this to the process ID of the process you wish to terminate
extern DWORD sigwait;     /*set this to the process ID of the process which is not
                            supposed to be interrupted*/
extern DWORD sigshutdown;
extern int ps_notimeincrement;
extern DWORD pfoccured;   /*set this to reset the pf_handler PCB, usually set by
                            the pf handler when a page fault has occured so that
                            its curent state does not get changed*/
extern DWORD sched_sysmes[3]; //[0] = pid, [1] = mes, [2] = data

extern PCB386 *schedp,*plast,*current_process,*next_process,curp;
extern PCB386 kernelPCB,schedpPCB;
extern PCB386 sPCB,pfPCB,pfPCB_copy,keyPCB;
extern FPUregs ps_fpustate,ps_kernelfpustate;


//calls the timer interrupt which in turn results to a call to the scheduler
extern void switchprocess();

void    addmemusage(process_mem **memptr,DWORD vaddr,DWORD pages);
void    addprocess(char *processname,process_mem *mem);  
int     broadcastmessage(DWORD sender,DWORD mes,DWORD data);
DWORD   createkthread(void *ptr,char *name,DWORD stacksize);
DWORD   createprocess(void *ptr,char *name,
                     DWORD *pagedir,process_mem *pmem,
                     void *stack,DWORD stacksize,
                     DWORD syscallsize,
                     void *dex32_signal,
                     char *params,
                     char *workdir,
                     PCB386 *parent);
DWORD   createthread(void *ptr,void *stack,DWORD stacksize);
DWORD   dex32_exitprocess(DWORD ret_value);
int     dex32_getname(DWORD processid,int bufsize,char *s);
void    dex32_getparametersinfo(char *buf);
DWORD   dex32_asyncproc(saveregs *r,void *entrypoint,char *name,DWORD stacksize);
DWORD   dex32_setservice();
void    dex32_set_timer(DWORD rate);
DWORD   dex32_killkthread(DWORD processid);
DWORD   dex32_killkthread_name(char *processname);
int     execp(char *fname,DWORD mode,char *params);
DWORD   exit(DWORD val);
PCB386  *findprocess(DWORD processid,int);
PCB386  *ps_findprocess(DWORD processid);
int     findprocessname(const char *name);
void   *findsemaphore(DWORD handle);
DWORD   fork(); /*NOT WORKING YET*/
DWORD   forkprocess(); /*NOT WORKING YET*/
DWORD   free_semaphore(DWORD handle);
void    freeprocessmemory(process_mem *memptr,DWORD *pagedir);
int     getmessage(DWORD *source,DWORD *mes,DWORD *data);
DWORD   getparentid();
int     get_processlist(PCB386 **buf);
DWORD   getprocessmemory(process_mem *memptr,DWORD *pagedir);
DWORD   getprocessid();
DWORD   get_semaphore(DWORD handle);
DWORD   kill_children(DWORD processid);
DWORD   kill_process(DWORD processid);
DWORD   kill_thread(PCB386 *ptr);
DWORD   set_semaphore(DWORD handle,DWORD val);
int     sendmessage(DWORD pid,DWORD mes,DWORD data);
int     sendmessageEX(DWORD source,DWORD pid,DWORD mes,DWORD data);
void    sleep(DWORD val);
DWORD   unloadprocess(DWORD processid);
void    halt();
PCB386 *scheduler(PCB386 *lastprocess);
void    taskswitcher();
void    show_process();
DWORD   totalprocess();
DWORD   getprocessinfo(DWORD processid,PCB386 *data);
DWORD   dex32_locktasks();
DWORD   dex32_unlocktasks();
DWORD   ps_dequeue(PCB386 *process);
DWORD   ps_enqueue(PCB386 *process);
PCB386 *ps_getcurrentprocess();
void    ps_user_kill(int pid);
void    signal(DWORD sigtype,void* ptr);
void    systemcall();
//process manager initialization function
void    process_init();
void 	ps_shutdown();
void copyprocessmemory(process_mem *memptr,process_mem **destmemptr);
void taskswitch();
void show_process_stat(int pid);
#endif
