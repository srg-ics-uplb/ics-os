
/*
 *   Name: DEX32 Process Management Module
 *   Copyright: 
 *   Author: Joseph Emmanuel Dayo
 *   Date: 09/11/03 04:11
 *   Description: Provides functions for process management and task switching
 
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

#include "process.h"

semaphore *semaphore_head;

/* The next process to be created will use this process ID
   pids 1-0x8A is reserved, pid 0 is the kernel pid*/
DWORD nextprocessid = 0x10;
    
//used for the busy waiting loops of the process manager.    
sync_sharedvar processmgr_busy;

int totalprocesses=0;

//global vriables used for triggering taskswitcher events.
DWORD sigpriority=0; //set this to the process ID of the process that requires immediate attention
DWORD sigterm=0; //set this to the process ID of the process you wish to terminate
DWORD sigwait=0; /*set this to the process ID of the process which is not
                   supposed to be interrupted*/
DWORD sigshutdown=0;
DWORD pfoccured=0; /*set this to reset the pf_handler PCB, usually set by
                     the pf handler when a page fault has occured so that
                    its curent state does not get changed*/
DWORD sched_sysmes[3]={0,0,0}; //[0] = pid, [1] = mes, [2] = data
int ps_notimeincrement = 0;
PCB386 *schedp,*plast,*current_process=0,*next_process=0,curp;
PCB386 kernelPCB,schedpPCB;
PCB386 sPCB,pfPCB,pfPCB_copy,keyPCB,mousePCB;
FPUregs ps_fpustate,ps_kernelfpustate;



//calls the timer interrupt which in turn results to
//a call to the scheduler
extern void switchprocess();



void ps_shutdown()
{
    printf("process manager: Shutdown not yet implemented\n");
};

void signal(DWORD sigtype,void* ptr)
{

};


DWORD getparentid()
{
    return current_process->owner;
};

DWORD getprocessid()
{
    return current_process->processid;
};

DWORD getpprocessid()
{
    return current_process->owner;
}; 

/*creates a USER thread (In DEX this is process which shares the same memory space as its parent
  process, but has its own stack pointer)*/
DWORD createthread(void *ptr,void *stack,DWORD stacksize)
{
    int pages;
    DWORD flags;
    
    PCB386 *temp=(PCB386*)malloc(sizeof(PCB386));
    memset(temp,0,sizeof(PCB386));

    totalprocesses++;
    
    dex32_stopints(&flags);
    
    temp->size=sizeof(PCB386);
    temp->before=current_process;
    
    sprintf(temp->name,"%s.thread",current_process->name);
    temp->processid   = nextprocessid++;
    temp->accesslevel = ACCESS_USER;
    temp->status     |= PS_ATTB_THREAD;
    current_process->childwait++;
    temp->meminfo     = current_process->meminfo;
    temp->owner       = getprocessid();
    temp->workdir     = current_process->workdir;
    temp->stdout      = current_process->stdout;
    temp->outdev      = current_process->outdev;
    temp->knext       = current_process->knext; 
    temp->pagedirloc  = current_process->pagedirloc;
    
    /*Set up initial contents of the CPU registers*/
    memset(temp,0,sizeof(saveregs));
    temp->regs.EIP    = (DWORD)ptr;
    temp->regs.ESP    = (DWORD)(stack+stacksize-4);
    temp->stackptr    = (void*)temp->regs.ESP;
    temp->regs.CR3    = (DWORD)current_process->pagedirloc;
    temp->regs.ES     = USER_DATA;
    temp->regs.SS     = USER_DATA;
    temp->regs.CS     = USER_CODE;
    temp->regs.DS     = USER_DATA;
    temp->regs.FS     = USER_DATA;
    temp->regs.GS     = USER_DATA;
    temp->regs.SS0    = SYS_STACK_SEL;

    //set up the initial stack pointer for system calls
    temp->stackptr0   = malloc(SYSCALL_STACK);
    temp->regs.ESP0   = temp->stackptr0+SYSCALL_STACK-4;
    temp->regs.EFLAGS = current_process->regs.EFLAGS;
    
    //initialize the current FPU state
    memcpy(&temp->regs2,&ps_kernelfpustate,sizeof(ps_kernelfpustate));
    
    //Tell the scheduler to add it to the process queue
    ps_enqueue(temp);
    
    dex32_restoreints(flags);

    return temp->processid;
};

//Tells the scheduler to queue a process
DWORD ps_enqueue(PCB386 *process)
{
    devmgr_scheduler_extension *cursched = extension_table[CURRENT_SCHEDULER].iface;
    bridges_link((devmgr_generic*)cursched,&cursched->ps_enqueue,
    process,0,0,0,0,0);
};

//Tells the scheduler to dequeue a process
DWORD ps_dequeue(PCB386 *process)
{
    devmgr_scheduler_extension *cursched = extension_table[CURRENT_SCHEDULER].iface;
    bridges_link((devmgr_generic*)cursched,&cursched->ps_dequeue,
    process,0,0,0,0,0);
};


//duplicates a process using COPY_ON_WRITE methods *NOT YET WORKING!!*
DWORD forkprocess(PCB386 *parent)
{
    int pages;
    DWORD *pagedir,pg,flags;
    DWORD parentpd = parent->pagedirloc;
    PCB386 *pcb = (PCB386*) malloc(sizeof(PCB386));
    
#ifdef DEBUG_FORK
    printf("fork process has been called.\n");
#endif

    dex32_stopints(&flags);
    memcpy(pcb,parent,sizeof(PCB386));
    strcat(pcb->name,".fork");
    totalprocesses++;
    pcb->size       = sizeof(PCB386);
    pcb->processid  = nextprocessid++;
    pcb->owner      = parent->processid;

    /*Allocate a new page directory*/
    pagedir=(DWORD*)mempop(); //obtain a physical address from the memory manager
    pg=(DWORD*)getvirtaddress((DWORD)pagedir); //convert physical address to a virtual address
    //initialize the new pagedirectory
    memset(pg,0,0x1000);

    pcb->regs.CR3   = pagedir;
    pcb->pagedirloc = pagedir;

#ifdef DEBUG_FORK
    printf("copying memory..\n");
#endif

    disablepaging();
    dex32_copy_pg(pagedir,parentpd);
    maplineartophysical((DWORD*)pagedir,(DWORD)SYS_PAGEDIR_VIR,(DWORD)pagedir    /*,stackbase*/,1);
    maplineartophysical((DWORD*)pagedir,(DWORD)SYS_PAGEDIR2_VIR,
    (DWORD)pagedir[SYS_PAGEDIR_VIR >> 22]&0xFFFFF000,1);
    maplineartophysical((DWORD*)pagedir,(DWORD)SYS_PAGEDIR3_VIR,
    (DWORD)pagedir[SYS_PAGEDIR_VIR >> 22]&0xFFFFF000,1);
    maplineartophysical((DWORD*)pagedir,(DWORD)SYS_KERPDIR_VIR,(DWORD)pagedir1    /*,stackbase*/,1);
    enablepaging();
    
#ifdef DEBUG_FORK
    printf("done. adding to process queue\n");
#endif

    //copies the memory allocation information of the parent to the child
    //(forked processes have the same virtual memory map at time of fork)
    copyprocessmemory(parent->meminfo,&pcb->meminfo);

    //add to the process list
    ps_enqueue(pcb);

    dex32_restoreints(flags);
    
#ifdef DEBUG_FORK
    printf("fork done.\n");
#endif

    return pcb->processid;
};


//The main procedure that is responsible for spawning all processes
DWORD createprocess(void *ptr,char *name, DWORD *pagedir, process_mem *pmem,
void *stack, DWORD stacksize, DWORD syscallsize, void *dex32_signal,
char *params, char *workdir, PCB386 *parent)
{
    int pages;
    DWORD flags , *pg;
    PCB386 *temp=(PCB386*)malloc(sizeof(PCB386));
    memset(temp,0,sizeof(PCB386));
    temp->before=current_process;
    strcpy(temp->name,name);
    totalprocesses++;
    temp->size         = sizeof(PCB386);
    temp->processid    = nextprocessid++;
    temp->accesslevel  = ACCESS_USER;
    temp->meminfo      = pmem;
    temp->owner        = parent->processid;
    temp->dex32_signal = dex32_signal;
    temp->op_success   = 1; 
    temp->arrivaltime  = getprecisetime(); 
    temp->stdin        = parent->stdin;
    memcpy(&temp->regs2,&ps_kernelfpustate,sizeof(ps_kernelfpustate));

    //get the working directory of this process
    if (workdir==0)
        temp->workdir  = parent->workdir;
    else
    {
        temp->workdir  = vfs_searchname(workdir);
        
        //validate workdir
        if (temp->workdir == 0) temp->workdir =parent->workdir;
    };
    //use the same screen as the one who called
    temp->outdev=parent->outdev;

    //find the parent process and increment it's waiting state
    parent->childwait++;

    temp->knext       = userheap; //set up the programs' initial break
    temp->pagedirloc  = pagedir;
    
    /*Set up the CPU registers*/
    memset(temp,0,sizeof(saveregs));
    temp->regs.EIP    = (DWORD)ptr;
    temp->regs.ESP    = (DWORD)stack;
    temp->stackptr    = (void*)temp->regs.ESP;
    temp->regs.CR3    = (DWORD)pagedir;
    temp->regs.ES     = USER_DATA;
    temp->regs.SS     = USER_DATA;
    temp->regs.CS     = USER_CODE;
    temp->regs.DS     = USER_DATA;
    temp->regs.FS     = USER_DATA;
    temp->regs.GS     = USER_DATA;
    temp->regs.SS0    = SYS_STACK_SEL;
    temp->syscallsize=syscallsize;
    //set up the program parameters
    if (params!=0)
    {
        temp->parameters=(char*)malloc(512);
        strcpy(temp->parameters,params);
    }
    else
        temp->parameters=0;


    sync_entercrit(&processmgr_busy);	
    dex32_stopints(&flags);  

    //allocate memory for the system call stack
    dex32_commitblock((DWORD)syscallstack,syscallsize,&pages,pagedir,PG_WR);
    addmemusage(&(temp->meminfo),syscallstack,pages);
    //set up the initial stack pointer
    temp->regs.ESP0=(syscallstack+syscallsize-4);
    //  temp->regs.ESP0=0x9FFFE;
    temp->regs.EFLAGS=0x200;
    temp->semhandle=0;



    //some functions that a character device uses

    disablepaging();
    dex32_copy_pagedirU(pagedir,pagedir1);
    enablepaging();
        
    pg = (DWORD*)getvirtaddress((DWORD)pagedir); /*convert to a virtual address so that
                                                 we could use it here without disabling
                                                 the paging mechanism of the 386*/

    maplineartophysical2((DWORD*)pg, (DWORD)SYS_PAGEDIR_VIR,(DWORD)pagedir    /*,stackbase*/,1);
    
    maplineartophysical2((DWORD*)pg, (DWORD)SYS_PAGEDIR2_VIR,
    (DWORD)pg[SYS_PAGEDIR_VIR >> 22]&0xFFFFF000,1);
    
    maplineartophysical2((DWORD*)pg, (DWORD)SYS_PAGEDIR3_VIR,
    (DWORD)pg[SYS_PAGEDIR_VIR >> 22]&0xFFFFF000,1);
    
    maplineartophysical2((DWORD*)pg, (DWORD)SYS_KERPDIR_VIR,(DWORD)pagedir1    /*,stackbase*/,1);

    //add to the list
    ps_enqueue(temp);

    dex32_restoreints(flags);
    sync_leavecrit(&processmgr_busy);

    return temp->processid;
};

DWORD dex32_asyncproc(saveregs *r,void *entrypoint,char *name,DWORD stacksize)
{
    PCB386 *temp=(PCB386*)malloc(sizeof(PCB386));

    temp->before=current_process;
    strcpy(temp->name,name);
    totalprocesses++;
    temp->processid=nextprocessid++;
    temp->accesslevel=ACCESS_SYS;
    temp->owner=1;
    temp->knext=knext;
    temp->pagedirloc=pagedir1;
    memset(temp,0,sizeof(saveregs));
    temp->regs.EIP=(DWORD)entrypoint;
    temp->stackptr=malloc(stacksize);
    temp->regs.ESP=(DWORD)(temp->stackptr+stacksize-4);
    temp->stackptr=(void*)temp->regs.ESP;
    temp->regs.CR3=(DWORD)pagedir1;
    temp->regs.EAX=r->EAX;
    temp->regs.EBX=r->EBX;
    temp->regs.ECX=r->ECX;
    temp->regs.EDX=r->EDX;
    temp->regs.ESI=r->ESI;
    temp->regs.EDI=r->EDI;
    temp->regs.ES=r->ES;
    temp->regs.SS=r->SS;
    temp->regs.CS=r->CS;
    temp->regs.DS=r->DS;
    temp->regs.FS=r->FS;
    temp->regs.GS=r->GS;
    temp->regs.EFLAGS=0x200;

    sync_entercrit(&processmgr_busy);
    //add to the list
    ps_enqueue(temp);
    sync_leavecrit(&processmgr_busy);
};

void ps_seterror(int error)
{
    current_process->lasterror =  error;
};

int ps_geterror()
{
    return current_process->lasterror;
};

//sends a message to all active processes
int broadcastmessage(DWORD sender,DWORD mes,DWORD data)
{
    PCB386 *ptr;
    int total,i;
    
    total = get_processlist(&ptr);
    
    for (i=0; i < total ; i++)
    {
        sendmessageEX(sender,ptr[i].processid,mes,data);
    } ;
    free(ptr);
    return 1;

};


int sendmessage(DWORD pid,DWORD mes,DWORD data)
{
    return sendmessageEX(current_process->processid,pid,mes,data);
};

//sends a message to another process
int sendmessageEX(DWORD source,DWORD pid,DWORD mes,DWORD data)
{
    DWORD cpuflags;
    PCB386 *ptr=(PCB386*)ps_findprocess(pid);
    if (ptr!=-1)
    {
        int index=ptr->meshead;

        if (ptr->mestotal>=MAX_MESSAGE-1) return IPCSTAT_FULL;
        
        dex32_stopints(&cpuflags);
        
        ptr->mesq[index].message=mes;
        ptr->mesq[index].data=data;
        ptr->mesq[index].sender=source;
        ptr->mesq[index].receiver=pid;

        ptr->mestotal++;
        ptr->meshead++;
        if (ptr->meshead>=MAX_MESSAGE) ptr->meshead=0;
        
        dex32_restoreints(cpuflags);
        
        return IPCSTAT_OK;
    };
    return IPCSTAT_ERROR;
};


int getmessage(DWORD *source,DWORD *mes,DWORD *data)
{
    PCB386 *ptr=current_process;
    DWORD cpuflags;
    
    int index=ptr->curmes;
    if (ptr->mestotal<=0) return 0;

    dex32_stopints(&cpuflags);

    *source=ptr->mesq[index].sender;
    *mes=ptr->mesq[index].message;
    *data=ptr->mesq[index].data;
    ptr->curmes++;
    ptr->mestotal--;
    if (ptr->mestotal<0) ptr->mestotal=0;
    if (ptr->curmes>=MAX_MESSAGE) ptr->curmes=0;
    
    dex32_restoreints(cpuflags);
    
    return 1;
};

/*creates a kernel thread*/
DWORD createkthread(void *ptr,char *name,DWORD stacksize)
{
    PCB386 *temp=(PCB386*)malloc(sizeof(PCB386));
    DWORD cpuflags;
    
    memset(temp,0,sizeof(PCB386));
    temp->before=current_process;
    strcpy(temp->name,name);
    totalprocesses++;
    temp->size        = sizeof(PCB386);
    temp->processid   = nextprocessid++;
    temp->accesslevel = ACCESS_SYS;
    temp->owner       = getprocessid();
    temp->status     |= PS_ATTB_THREAD; 
    temp->knext       = knext;
    temp->pagedirloc  = pagedir1;
    temp->workdir     = current_process->workdir;

    //set up the initial values of the CPU registers for this process
    memset(temp,0,sizeof(saveregs));
    temp->regs.EIP    = (DWORD)ptr;
    temp->stackptr    = malloc(stacksize);
    temp->regs.ESP    = (DWORD)(temp->stackptr+stacksize-4);
    temp->stackptr    = (void*)temp->regs.ESP;
    temp->regs.CR3    = (DWORD)pagedir1;
    temp->regs.ES     = SYS_DATA_SEL;
    temp->regs.SS     = SYS_STACK_SEL;
    temp->regs.CS     = SYS_CODE_SEL;
    temp->regs.DS     = SYS_DATA_SEL;
    temp->regs.FS     = SYS_DATA_SEL;
    temp->regs.GS     = SYS_DATA_SEL;
    temp->regs.EFLAGS = 0x200;

    temp->arrivaltime = getprecisetime(); 
    temp->stdin       = current_process->stdin;

    /*critical section...*/
    sync_entercrit(&processmgr_busy);
    dex32_stopints(&cpuflags);
    
    //add to the process list
    ps_enqueue(temp);

    dex32_restoreints(cpuflags);
    
    //end of critical section
    sync_leavecrit(&processmgr_busy);
    
    return temp->processid;
    ;
};

int ps_changename(const char *name,int pid)
{
    PCB386 *ptr;
    sync_entercrit(&processmgr_busy);

    ptr=ps_findprocess(pid);
    if (ptr!=-1) strcpy(ptr->name,name);

    sync_entercrit(&processmgr_busy);
};

//decrements the wait status of the parent
//used for creating services
DWORD dex32_setservice()
{
    PCB386 *ptr;
    sync_entercrit(&processmgr_busy);
    ptr=ps_findprocess(current_process->owner);
    if (ptr!=-1)
    {
        ptr->childwait=0;
    };
    sync_leavecrit(&processmgr_busy); 
};

DWORD dex32_exitprocess(DWORD ret_value)
{
    dex32_killkthread(current_process->processid);
    while (1);
    ;
};

//used to kill kernel(Ring0) threads only!!!
DWORD dex32_killkthread(DWORD processid)
{

    PCB386 *ptr;
    PCB386 *end;
    DWORD flags;
    sync_entercrit(&processmgr_busy);

    ptr = bridges_ps_findprocess(processid);

    dex32_stopints(&flags);
    if (ptr != -1)
    {
        if (!ptr->status&PS_ATTB_UNLOADABLE)
        {
            PCB386 *parent;
            if (ptr->accesslevel == ACCESS_SYS)
                free(ptr->stackptr);
                
            if (ptr->semhandle != 0)
                set_semaphore(ptr->semhandle,SIG_TERM);
                
            ps_dequeue(ptr);
            free(ptr);
            dex32_restoreints(flags);

            sync_leavecrit(&processmgr_busy);
            return 1;
        };
    };

    dex32_restoreints(flags);
    sync_leavecrit(&processmgr_busy);
    return 0;
}

/*puts the current process to sleep for a desired amount of time
  in milliseconds*/
void sleep(DWORD val)
{
    current_process->waiting = val;
};

/*called when another process wants to kill another.
  Also performs garbage collection (reclaims memory 
  used by the application).*/
DWORD kill_process(DWORD processid)
{
    PCB386 *ptr,*parentptr=0;
    sync_entercrit(&processmgr_busy);
    ptr = bridges_ps_findprocess(processid);
    if (ptr!=-1)
    {

        if (! (ptr->status&PS_ATTB_UNLOADABLE) )
        {
            PCB386 *parent;
            kill_children(processid); //kill child processes first

            if (ptr->accesslevel == ACCESS_SYS) //a kernel thread?
            {
                dex32_killkthread(ptr);
                sync_leavecrit(&processmgr_busy);
                return 1;
            };

            if ( ptr->status&PS_ATTB_THREAD ) //a thread process? If yes then redirect to another procedure
            {
                kill_thread(ptr);
                sync_leavecrit(&processmgr_busy);
                return 1;
            };

            while (closeallfiles(ptr->processid)==1);

            parent=ps_findprocess(ptr->owner);
            if (parent!=-1)
            {
                parent->childwait=0;
            };


            //locate the parent process and decrement its waiting
            //status...important for the dex32_wait() function

            if (ptr->accesslevel == ACCESS_SYS)
                free(ptr->stackptr);
            
            /*Perform memory garbage collection if necessary*/
            if (ptr->meminfo!=0)
                freeprocessmemory(ptr->meminfo,(DWORD*)ptr->pagedirloc);

            if (!(ptr->status&PS_ATTB_THREAD) && (ptr->accesslevel != ACCESS_SYS) )
            {
                //free the page tables used by the application
                dex32_freeuserpagetable((DWORD*)ptr->pagedirloc);
#ifdef MEM_LEAK_CHECK
                printf("1 page freed (page directory).\n");
#endif
                mempush(ptr->pagedirloc);
            };
            
            if (ptr->parameters!=0) free(ptr->parameters);

            if (ptr->stdout!=0) {
                free(ptr->stdout);
            };

            //Tell the scheduler to remove this process from the queue
            ps_dequeue(ptr);
            free(ptr);
            sync_leavecrit(&processmgr_busy);
            return 1;
        };
    }; 
    sync_leavecrit(&processmgr_busy);
    return 0;

};


//used to kill kernel threads
DWORD kill_thread(PCB386 *ptr)
{
    DWORD flags;
    dex32_stopints(&flags);

    kill_children(ptr->processid); //kill the children of this thread first!!
    
    //Tell the scheduler to remove this process from the process queue
    ps_dequeue(ptr);

    if (ptr->stackptr0!=0)
        free(ptr->stackptr0);
    free(ptr);
    dex32_restoreints(flags);
    return 1;
    ;
};

//called when another process wants to kill another
DWORD kill_children(DWORD processid)
{
    PCB386 *ptr;
    sync_entercrit(&processmgr_busy);

    ptr = bridges_ps_findprocess(processid);
    if (ptr!=-1)
    {
    if (ptr->owner==processid && !( ptr->status&PS_ATTB_UNLOADABLE ) )
        {
            kill_thread(ptr);
            sync_leavecrit(&processmgr_busy);
            return 1;
        };
     };

    sync_leavecrit(&processmgr_busy); 
    return 0;

};

//called when a process wishes to terminate itself
DWORD exit(DWORD val)
{
    DWORD flags;

    //close all files the process has opened
    closeallfiles(current_process->processid);
    
    /* tell the task switcher to kill this process by setting
       the sigterm global varaible to the current pid. If sigeterm is non-zero
       the taskswitcher terminates the process with pid equal to sigterm*/
    sigterm=current_process->processid;
    
    taskswitch();
    while (1);
    return 0;
    
};

/*kills a process with the specified pid, leaves all files
  open*/
void ps_user_kill(int pid)
{
    if (ps_findprocess(pid)!=-1)
        {
              sigterm = pid;
              taskswitch();  
        };
};

DWORD suspendsem=0,semactive=0;

//creates a semaphore to be used by the system
DWORD create_semaphore(DWORD val)
{
    semaphore *sem=(DWORD*)malloc(sizeof(semaphore));
    //generate a handle value for the semaphore, the semaphores
    //location in memory should suffice
    sem->handle=(DWORD)sem;
    //insert the sempahore into memory
    sem->owner=current_process->processid;
    sem->data=val;
    //This is a critical section so we must stop other processes
    //from creating their own semaphores in the middle of
    //creating this semaphore!
    while (semactive);
    while (suspendsem);
    suspendsem=1;
    sem->next=semaphore_head->next;
    semaphore_head->next->prev=sem;
    semaphore_head->next=sem;
    sem->prev=semaphore_head;
    suspendsem=0;
    ;
};

/*====================================================================
int get_processlist(PCB386 **buf)

* places the list of processes to buf, get_processlist is responsible
* for allocating the necessary size required to store the list 
* Returns: The total nubmer of processes in buf
*/
int get_processlist(PCB386 **buf)
{
    int total;
    devmgr_scheduler_extension *cursched;
        
    cursched = (devmgr_scheduler_extension*)extension_table[CURRENT_SCHEDULER].iface;
   
    if (cursched->ps_listprocess == 0)
    {
        printf("ERROR: schduler extension module does not support ps_listprocess()..\n");
        return 0;
    };
    //begin mutual exclusion
    sync_entercrit(&processmgr_busy);
    
    total = bridges_call((devmgr_generic*)cursched,&cursched->ps_listprocess,0,0,0);
    *buf = (PCB386*) malloc( sizeof(PCB386) * total );
    
    //make an intermodule call to the scheduler
    bridges_call((devmgr_generic*)cursched,&cursched->ps_listprocess,*buf,sizeof(PCB386),total);
    
    sync_leavecrit(&processmgr_busy);
    
    return total;
};

//returns the processid of a process based on its name
int findprocessname(const char *name)
{
    PCB386 *ptr;
    int total, i,retval = -1;
    
    total = get_processlist(&ptr);
    
    sync_entercrit(&processmgr_busy);
    
    for (i = 0; i < total ; i++)
    {
        if ( strcmp(ptr[i].name,name) == 0 )
            {
                 retval = ptr[i].processid;       
                 break;
            };    
    };
    
    sync_leavecrit(&processmgr_busy);
    
    free(ptr);
    return retval;
};

//returns the PCB386 structure of a process based on its process id
PCB386 *ps_findprocess(DWORD processid)
{
    PCB386 *ptr;
    PCB386 *end;
    DWORD flags;
    int total,i;
    sync_justwait(&processmgr_busy);
    return bridges_ps_findprocess(processid);
    return (PCB386*)-1;
    ;
};

int dex32_getname(DWORD processid,int bufsize,char *s)
{
    PCB386 *ptr;
    sync_justwait(&processmgr_busy);

    ptr = ps_findprocess(processid);
    if (ptr!=-1)
    {
        int i;
        //copy the name of the process to s
        for (i=0;i<bufsize&&ptr->name[i];i++)
            s[i]=ptr->name[i];
        s[i]=0;
        return 1;
    };

    return 0;
};
//copies the parameters passed to a program into a user buffer
void dex32_getparametersinfo(char *buf)
{
    if (current_process->parameters!=0)
        strcpy(buf,current_process->parameters);
    else
        strcpy(buf,"");
};

//waits till a child process terminates
int dex32_wait()
{
    int waitval;
    if (current_process->childwait==0) return 1;
    waitval=current_process->childwait;
    while (current_process->childwait>=waitval&&current_process->childwait!=0);
};

int dex32_waitpid(int pid,int status)
{
    while (ps_findprocess(pid) != -1);  
};


void *findsemaphore(DWORD handle)
{
    semaphore *ptr=semaphore_head;
    do
    {
        if (ptr->handle==handle)
        {
            return (void*)ptr;
            ;
        };
        ptr=ptr->next;
    } 
    while (ptr!=semaphore_head);
    return 0;
    ;
};

//gets the value stored in a semaphore
DWORD get_semaphore(DWORD handle)
{
    //search semaphores for a match
    semaphore *ptr;
    while (suspendsem);
    semactive=1;
    ptr=(semaphore*)findsemaphore(handle);
    if (ptr==0) {
        semactive=0;
        return 0;
    };
    semactive=0;
    return ptr->data;
    ;
};

//sets a value stored in a semaphore
DWORD set_semaphore(DWORD handle,DWORD val)
{
    semaphore *ptr;
    while (suspendsem);
    semactive=1;
    ptr=(semaphore*)findsemaphore(handle);
    if (ptr==0) {
        semactive=0;
        return 0;
    };
    semactive=0;
    return (ptr->data=val);
    ;
};

DWORD free_semaphore(DWORD handle)
{
    semaphore *ptr;
    while (suspendsem);
    semactive=1;
    ptr=(semaphore*)findsemaphore(handle);
    if (ptr==0) {
        semactive=0;
        return 0;
    };
    semactive=0;
    //reconnect the missing pieces
    ptr->prev->next=ptr->next;
    ptr->next->prev=ptr->prev;
    free(ptr);
    return 1;
    ;
};




DWORD dex32_killkthread_name(char *processname)
{
    PCB386 *ptr;
    int total, processid , i;
    total = get_processlist(&ptr);
    
    for (i=0; i < total; i++)
    {
        if (strcmp(ptr[i].name, processname) == 0 && !(ptr[i].status&PS_ATTB_UNLOADABLE) )
        {
                sigterm=ptr[i].processid;
                free(ptr);
                return 1;
        };
    };
    
    //maybe the paramater given was a pid?
    processid = atoi(processname);

    for (i=0; i < total; i++)
    {
        if ( (ptr[i].processid == processid) && !(ptr[i].status&PS_ATTB_UNLOADABLE) )
        {
                sigterm=ptr[i].processid;
                free(ptr);
                return 1;
        };
    };

    return 0;
}

//adds memory usage to a processes memory descriptor
void addmemusage(process_mem **memptr,DWORD vaddr,DWORD pages)
{
    process_mem *ptr=(process_mem*)malloc(sizeof(process_mem));
    ptr->next=*memptr;
    *memptr=ptr;
    ptr->vaddr=vaddr;
    ptr->pages=pages;
};

/*This function determines the amount of memory a process
 *   is currently using*/
DWORD getprocessmemory(process_mem *memptr,DWORD *pagedir)
{
    DWORD total_memory=0;
    process_mem *tmpr=memptr; //point to the head
    while (tmpr->vaddr!=0&&tmpr!=0)
    {
        process_mem *tmpr2=tmpr->next;
        total_memory+=getmultiple((void*)tmpr->vaddr,pagedir,tmpr->pages);
        tmpr=tmpr2;
    };
    return total_memory;
};

void freeprocessmemory(process_mem *memptr,DWORD *pagedir)
{
    process_mem *tmpr=memptr;//point to the head
#ifdef MEM_LEAK_CHECK
    printf("freeprocessmemory called.\n");
#endif
    do
    {
        process_mem *tmpr2=tmpr->next;
        freemultiple((void*)tmpr->vaddr,pagedir,tmpr->pages);
        free(tmpr);
        tmpr=tmpr2;
    }
    while (tmpr!=0);
#ifdef MEM_LEAK_CHECK
    printf("freeprocessmemory ended.\n");
#endif
};

/*Copies memory usage information. Used by the fork() command*/
void copyprocessmemory(process_mem *memptr,process_mem **destmemptr)
{
    process_mem *tmpr=memptr;//point to the head
    while (tmpr->vaddr!=0&&tmpr!=0)
    {
        process_mem *tmpr2=tmpr->next;
        addmemusage(destmemptr, tmpr->vaddr,tmpr->pages);
        tmpr=tmpr2;
    };
};

void halt()
{
    while (1);
};


//switched to another process using the TSS switching method
// 1/25/2004: Also added the capability to save the FPU registers to prevent
//            applications that use the FPU from doing unexpected things
void ps_switchto(PCB386 *process)
{
    //set the state of the floating point unit from the PCB of the
    //process to switch to
    memcpy(&ps_fpustate,&process->regs2,sizeof(FPUregs));
    asm volatile ("frstor ps_fpustate");

    //switch to a user process
    if (process->accesslevel==ACCESS_USER)
    {
        dex32_setbase(USER_TSS,process);
        switchuserprocess();
        setattb(USER_TSS,0xE9); //run a user process
    }
    else
        //switch to a kernel mode process
    {
        dex32_setbase(SYS_TSS,process);
        switchprocess();
        setattb(SYS_TSS,0x89); //run a kernel process
    };

    //save the state of the floating point unit
    asm volatile ("fnsave ps_fpustate");
    memcpy(&process->regs2,&ps_fpustate,sizeof(FPUregs));
};

/*Calls the timer interrupt which calls the taskswitcher*/
void taskswitch()
{
   //Tell the taskswitcher not to increment the time
   ps_notimeincrement = 1; 
   asm volatile ("int $0x20");
};

//The taskswitcher is basically the program that runs all the time.
//It is  responsible for switching ro various processes and terminating them.
void taskswitcher()
{
    char temp[255];
    DWORD cputime=0;
    PCB386 *readyprocess;
    do
    {
        stopints();
        //fetch a ready process from the ready queue

        do {
            if (!sigwait)
                {
                //if the wait register is not set, switch to
                //another process, the wait is used to prevent
                //other processes from taking control during
                //a critical section, sigwait should be
                //returned to its original state after the critical
                //section is over

                //obtain next job from the scheduler
                //readyprocess=extension_current->ps_scheduler->scheduler(&kernelPCB);
                devmgr_scheduler_extension *cursched = (devmgr_scheduler_extension*)extension_table[CURRENT_SCHEDULER].iface;
                readyprocess = (PCB386*)bridges_link((devmgr_generic*)cursched,&cursched->scheduler,
                current_process,0,0,0,0,0);
                };
                //readyprocess=bridges_ps_scheduler(current_process);
            
            current_process=readyprocess;

            //tell the cpu to give control to <readyprocess>
            ps_switchto(readyprocess);

            /*Make sure the taskwitcher was really called by the timer, since
              another way of calling the taskswithcer is through taskswitch().
              In that case we must not call time_handler in order to make
              the time as accurate as possible*/
            
            
            if (!ps_notimeincrement)
            {
                //Increment system time... etc.
                time_handler();
            }
                else
            ps_notimeincrement = 0;
            
            //record the number of milleseconds the process used
            readyprocess->totalcputime++;
            
            //A process wants to get immediate control, usually set by
            //device drivers that are hooked to IRQs
            if (sigpriority)
            {
                PCB386 *priorityprocess = ps_findprocess(sigpriority);
                if (priorityprocess!=-1)
                {
                    //give the process to the CPU       
                    ps_switchto(priorityprocess);       
                };
                sigpriority = 0;
            };

        } 
        while (sigwait);

        //check the registers, and act if necessary

        if (pfoccured)
        {
            setattb(PF_TSS,0x89);
            memcpy(&pfPCB.regs,&pfPCB_copy.regs,sizeof(pfPCB.regs));
            pfoccured=0;
        };

        if (sigterm&&!flushing)
        {
            flushok=0;
            kill_process(sigterm);
            flushok=1;
            sigterm=0;
            ;
        };

        if (sched_sysmes[0])
        {
            sendmessage(sched_sysmes[0],sched_sysmes[1],sched_sysmes[2]);
            sched_sysmes[0]=0;
            ;
        };

        if (sigshutdown)
        {
            broadcastmessage(1,SIG_KILL,0);
            ;
        };

    } 
    while (1);

    ;
};


void show_process_stat(int pid)
{
    int total,i;
    PCB386* ptr;
    char temp[20],temp1[20],temp2[20],temp3[20];
    total = get_processlist(&ptr);    
    for (i=0;i<total;i++)
       {
              if (pid ==  ptr[i].processid)
                 {
                         printf("=========================================================\n");
                         printf("Name:%s", ptr[i].name);
                         printf("Parent:%d", ptr[i].owner);
                         printf("EIP=0x%s\n",itoa(ptr[i].regs.EIP,temp,16));
                         printf("EAX=0x%s EBX=0x%s ECX=0x%s EDX=0x%s\n",itoa(ptr[i].regs.EAX,temp,16),
                         itoa(ptr[i].regs.EBX,temp1,16),itoa(ptr[i].regs.ECX,temp2,16),
                         itoa(ptr[i].regs.EDX,temp3,16));

                         printf("EDI=0x%s ESI=0x%s ESP=0x%s Flags=0x%s\n",itoa(ptr[i].regs.EDI,temp,16),
                         itoa(ptr[i].regs.ESI,temp1,16),itoa(ptr[i].regs.ESP,temp2,16),
                         itoa(ptr[i].regs.EFLAGS,temp3,16));
                         
                         printf("waiting: %d\n", ptr[i].waiting);
                         printf("last system calls:(1) : 0x%s ,(2-last): 0x%s\n",
                         itoa(ptr[i].cursyscall[0],temp2,16),itoa(ptr[i].cursyscall[1],temp,16));

                 };
       };
};

/*An auxillary function for qsort for comparing two elements*/
int process_pid_sorter(PCB386 *n1,PCB386 *n2)
{
    if (n1->processid > n2->processid) return 1;
    if (n2->processid > n1->processid) return -1;
    return 0;
};


//displays the list of processes
void show_process()
{
    int total=0,i;
    DWORD totalsize=0 , grandtotalcputime = 0;
    PCB386* ptr;
    char levelstr[13];
        
    textbackground(BLUE);
    printf("dex32_scheduler  v1.00\n");
    textbackground(BLACK);
    printf("Processes in memory:\n\n");
    textcolor(MAGENTA);
    printf("%-5s %-17s %-10s %-17s %6s %5s %10s\n","ID","Name","User Level","Owner","Size","AT","CT");
    textcolor(WHITE);

    
   
    /*Tell the scheduler to give us an array of PCBs which contain the PCBs of the processes
      running in the system*/  
    total = get_processlist(&ptr);
    
    qsort(ptr,total,sizeof(PCB386),process_pid_sorter);
    /*first we obtain the total cputime of all the processes, this is computed by
      the summation of the delta cputimes*/
      
    for (i=0; i<total; i++) grandtotalcputime += ( ptr[i].totalcputime - ptr[i].lastcputime );

    
    for (i=0; i<total; i++)
    {
        char temp[255];
        PCB386 *ps;
        int percent_cpu_time;
        //obtain the size of the memory used by the process (no. of pages used)
        DWORD psize=getprocessmemory(ptr[i].meminfo,ptr[i].pagedirloc);	

        //convert to Kilobytes
        psize=((psize*0x1000)/1024)+4;

        //update the global total
        totalsize+=psize;
        
        printf("[%-3s]",itoa(ptr[i].processid,temp,10));
        if ( ptr[i].status & PS_ATTB_UNLOADABLE ) textcolor(RED);
        else
        if (ptr[i].accesslevel == ACCESS_SYS) textcolor(LIGHTBLUE);
        else
            textcolor(GREEN);
            
        printf(" %-17s",ptr[i].name);
        textcolor(WHITE);
        strcpy(levelstr,"?");

        //determine the access level and then show it on the screen
        if (ptr[i].accesslevel == ACCESS_SYS) strcpy(levelstr,"supervisor");
        else
            if (ptr[i].accesslevel == ACCESS_USER) strcpy(levelstr,"user");

        //obtain the name of the parent process
        dex32_getname(ptr[i].owner,sizeof(temp),temp);

        /*compute for the percent CPU time*/
        percent_cpu_time = (ptr[i].totalcputime - ptr[i].lastcputime) * 100 / grandtotalcputime;
        
        sync_entercrit(&processmgr_busy);
        
        ps = ps_findprocess(ptr[i].processid);
        if (ps!=-1)
        {
        ps->lastcputime = ptr[i].totalcputime;
        };
        
        sync_leavecrit(&processmgr_busy);
        
        printf(" %-10s %-17s %5dK %5ds %5ds(%2d)%%\n",levelstr,temp,psize,
        ptr[i].arrivaltime/100, ptr[i].totalcputime/100, percent_cpu_time);
    };

    printf("\nTotal            : %d processes (%d KB)\n",total,totalsize);
    printf("Time Since Startup : %d\n", ticks / context_switch_rate);
    printf("Legend: AT = Arrival Time, CT = CPU Time, %%CT = Percent CPU Time\n");
    
    free(ptr);
};


DWORD totalprocess()
{
    int total=0;
    PCB386* ptr;
    
    total = get_processlist(&ptr);
    free(ptr);
    return total;
};

DWORD getprocessinfo(DWORD processid,PCB386 *data)
{
    PCB386 *ptr;
    sync_justwait(&processmgr_busy);
    ptr = ps_findprocess(processid);

    if (ptr!=-1)
    {
        memcpy(data,ptr,ptr->size);
        return 1;
    };
    return 0;
    ;
};

//dex32_locktasks is used only by system functions to temporarily prevent
//other processes from taking control of the CPU
DWORD dex32_locktasks()
{
    sigwait=1;
    return 1;
};

DWORD dex32_unlocktasks()
{
    sigwait=0;
    return 1;
    ;
};

extern loadtsr();



void systemcall()
{
    printf("A system call has been called\n");
    ;
};


/* This procedure gets called when the kernel boots up, it sets up
   the initial processes that would be run.*/
void process_init()
{
    char tmp[255];
    PCB386 *kernel;
    
    
    //initialize the FPU
    asm volatile ("fninit");
    asm volatile ("fnsave ps_kernelfpustate");
    
    //add the first process in memory which is the process kernel
    kernel=&sPCB;
    memset(kernel,0,sizeof(PCB386));
    kernel->next=kernel;
    kernel->before=kernel;
    kernel->processid=0;
    kernel->meminfo=0;
    strcpy(kernel->name,"dex_kernel-beta");
    kernel->accesslevel=ACCESS_SYS;
    kernel->status = PS_ATTB_LOCKED | PS_ATTB_UNLOADABLE;
    kernel->knext=knext;
    kernel->outdev=consoleDDL;
    kernel->pagedirloc=pagedir1;

    //initialize the current FPU state
    memcpy(&kernel->regs2,&ps_kernelfpustate,sizeof(ps_kernelfpustate));
    
    memset(&kernel->regs,0,sizeof(saveregs));
    kernel->regs.EIP=(DWORD)dex_init;
    kernel->regs.ESP= DISPATCHER_STACK_LOC;
    kernel->regs.CR3=pagedir1;
    kernel->regs.ES=SYS_DATA_SEL;
    kernel->regs.SS=SYS_STACK_SEL;
    kernel->regs.CS=SYS_CODE_SEL;
    kernel->regs.DS=SYS_DATA_SEL;
    kernel->regs.FS=SYS_DATA_SEL;
    kernel->regs.GS=SYS_DATA_SEL;
    kernel->regs.ESP0= DISPATCHER_STACK_LOC;
    kernel->regs.SS0= SYS_DATA_SEL;
    kernel->regs.EFLAGS=0x200;
    memcpy(&kernelPCB,kernel,sizeof(PCB386));
    setgdt(SYS_TSS,&kernelPCB.regs,103,0x89,0);
    sched_phead=kernel;

    /*Set up the PCB of the scheduler*/
    schedp=&schedpPCB;
    memset(schedp,0,sizeof(PCB386));
    
    schedp->processid=1;
    strcpy(schedp->name,"dex32_sched");
    schedp->accesslevel=ACCESS_SYS;
    schedp->status = PS_ATTB_LOCKED | PS_ATTB_UNLOADABLE;
    schedp->knext=knext;
    schedp->pagedirloc=pagedir1;
    schedp->outdev = consoleDDL;
    schedp->regs.EIP=(DWORD)taskswitcher;
    schedp->regs.ESP= SCHED_STACK_LOC;
    schedp->regs.ES=SYS_DATA_SEL;
    schedp->regs.SS=SYS_STACK_SEL;
    schedp->regs.CS=SYS_CODE_SEL;
    schedp->regs.DS=SYS_DATA_SEL;
    schedp->regs.FS=SYS_DATA_SEL;
    schedp->regs.CR3=pagedir1;
    schedp->regs.GS=SYS_DATA_SEL;
    schedp->regs.EFLAGS=0;
    schedp->regs.SS0=SYS_STACK_SEL;
    schedp->regs.ESP0= SCHED_STACK_LOC;
    setgdt(SCHED_TSS,&(schedp->regs),103,0x89,0);

    loadtsr();

    keyPCB.next=0;
    keyPCB.processid=1;
    strcpy(keyPCB.name,"keybhandler");
    keyPCB.accesslevel=ACCESS_SYS;
    keyPCB.priority=0;
    keyPCB.status = PS_ATTB_LOCKED | PS_ATTB_UNLOADABLE;
    keyPCB.knext=knext;
    keyPCB.pagedirloc=pagedir1;
    keyPCB.outdev= consoleDDL;
    memset(&keyPCB.regs,0,sizeof(saveregs));
    keyPCB.regs.EIP=(DWORD)kbdwrapper;
    keyPCB.regs.ESP= PAGEFAULT_STACK_LOC;
    keyPCB.regs.ES=SYS_DATA_SEL;
    keyPCB.regs.SS=SYS_STACK_SEL;
    keyPCB.regs.CS=SYS_CODE_SEL;
    keyPCB.regs.DS=SYS_DATA_SEL;
    keyPCB.regs.FS=SYS_DATA_SEL;
    keyPCB.regs.CR3=pagedir1;
    keyPCB.regs.GS=SYS_DATA_SEL;
    keyPCB.regs.EFLAGS=0;
    keyPCB.regs.SS0=SYS_STACK_SEL;
    keyPCB.regs.ESP0 = PAGEFAULT_STACK_LOC;
    setgdt(KEYB_TSS,&(keyPCB.regs),103,0x89,0);

    mousePCB.next=0;
    mousePCB.processid=1;
    strcpy(mousePCB.name,"mousehandler");
    mousePCB.accesslevel=ACCESS_SYS;
    mousePCB.priority=0;
    mousePCB.status = PS_ATTB_LOCKED | PS_ATTB_UNLOADABLE;
    mousePCB.knext=knext;
    mousePCB.pagedirloc=pagedir1;
    mousePCB.outdev= consoleDDL;
    memset(&mousePCB.regs,0,sizeof(saveregs));
    mousePCB.regs.EIP=(DWORD)mousewrapper;
    mousePCB.regs.ESP= PAGEFAULT_STACK_LOC;
    mousePCB.regs.ES=SYS_DATA_SEL;
    mousePCB.regs.SS=SYS_STACK_SEL;
    mousePCB.regs.CS=SYS_CODE_SEL;
    mousePCB.regs.DS=SYS_DATA_SEL;
    mousePCB.regs.FS=SYS_DATA_SEL;
    mousePCB.regs.CR3=pagedir1;
    mousePCB.regs.GS=SYS_DATA_SEL;
    mousePCB.regs.EFLAGS=0;
    mousePCB.regs.SS0=SYS_STACK_SEL;
    mousePCB.regs.ESP0 = PAGEFAULT_STACK_LOC;
    setgdt(MOUSE_TSS,&(mousePCB.regs),103,0x89,0);


    /*set up the PCB of the pagefault handler*/
    pfPCB.next=0;
    pfPCB.processid=1;
    strcpy(pfPCB.name,"dex32_pfhandler");
    pfPCB.accesslevel=ACCESS_SYS;
    pfPCB.priority=0;
    pfPCB.status = PS_ATTB_LOCKED | PS_ATTB_UNLOADABLE;
    pfPCB.knext=knext;
    pfPCB.pagedirloc=pagedir1;
    memset(&pfPCB.regs,0,sizeof(saveregs));
    pfPCB.regs.EIP=(DWORD)pfwrapper;
    pfPCB.regs.ESP=0x7FFFE;
    pfPCB.regs.ES=SYS_DATA_SEL;
    pfPCB.regs.SS=SYS_STACK_SEL;
    pfPCB.regs.CS=SYS_CODE_SEL;
    pfPCB.regs.DS=SYS_DATA_SEL;
    pfPCB.regs.FS=SYS_DATA_SEL;
    pfPCB.regs.CR3=pagedir1;
    pfPCB.regs.GS=USER_DATA;
    pfPCB.regs.EFLAGS=0;
    setgdt(PF_TSS,&(pfPCB.regs),103,0x89,0);
    //create a duplicate copy
    memcpy(&pfPCB_copy.regs,&pfPCB.regs,sizeof(pfPCB.regs));

    setgdt(USER_CODE,0,0xFFFFF,0xFA,0xCF);
    setgdt(USER_DATA,0,0xFFFFF,0xF2,0xCF);
    setgdt(USER_STACK,0,0xFFFFF,0xF2,0xCF);
    setgdt(USER_TSS,0,103,0xE9,0);
    setcallgate(DEX_SYSCALL,SYS_CODE_SEL,systemcall,0,3);

    //set up the semaphore manager
    semaphore_head=(semaphore*)malloc(sizeof(semaphore));
    semaphore_head->owner=0;
    semaphore_head->next=0;
    semaphore_head->prev=0;

    //initialize current_process
    current_process = kernel;
    current_process->workdir = vfs_root;

    ps_scheduler_install();

#ifdef DEBUG_STARTUP
    printf("process manager: done.\n");
#endif

    processmgr_busy.busy = 0;
    processmgr_busy.wait = 0;
    
    printf("starting process manager...\n");
};

