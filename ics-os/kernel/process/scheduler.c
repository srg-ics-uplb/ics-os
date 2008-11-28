/*
Name: Dex32 Default Round-Robin Scheduler
Author: Joseph Emmanuel Dayo
Description: This module is the default round-robin scheduler that is
				 initially used by the operating system
				 
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

#include "../dextypes.h"
#include "process.h"
#include "scheduler.h"
#include "../devmgr/dex32_devmgr.h"

PCB386 *sched_phead;
int ps_schedid;
devmgr_scheduler_extension ps_scheduler;


//Currently Implements the Round-Robin Algorithm
PCB386 *scheduler(PCB386 *lastprocess)
{
  PCB386 *ptr = lastprocess->next;
  
  //if this process is blocked or is waiting, we get another process.
  //This assumes that at least one process is not blocked or waiting.
  while ( (ptr->status & PS_ATTB_BLOCKED) || ptr->waiting )
  {
      ptr=ptr->next;
      if (ptr->waiting) ptr->waiting--;
  };    
 //we should have picked a process at this point. 
 return ptr;
};


//This is called when the extension manager is ready to make the current
//scheduler active
int sched_attach(devmgr_generic *cur)
{
    devmgr_scheduler_extension *oldsched=(devmgr_scheduler_extension*)cur;
    //get the location of the PCB head 
    sched_phead = oldsched->ps_gethead();
};

PCB386 *sched_getcurrentprocess()
{
   return current_process;
};

PCB386 *sched_gethead()
{
   return sched_phead;
};

//adds a process to a circular doubly-linked list process queue
void sched_enqueue(PCB386 *process)
{
	 PCB386 *temp;
	 process->size = sizeof(PCB386);
	 
     //no processes in memory yet?
     if (sched_phead==0)
        {
        	sched_phead = process;
            //fill up phead's connections
            sched_phead->next = sched_phead;
            sched_phead->before = sched_phead;
        }
     else
        {
        	//Use insert at head method
          	temp = sched_phead->next;
            //fill up phead's connections
            sched_phead->next = process;

            //fill up process's connections
            process->next = temp;
            process->before = sched_phead;

            //fill up temp's connections
            temp->before = process;
        };
};

//removes a process with the specified pid from a doubly-linked list process queue
int sched_dequeue(PCB386 *ptr)
{
      ptr->before->next=ptr->next;
      ptr->next->before=ptr->before;
      return 1;
      
};

/*Unlike sched_listprocess, sched_findprocess should return the pointer
to the actual PCB structure it uses.  The DEX process manager may use this
to modify the actual PCB of the scheduler*/
PCB386 *sched_findprocess(int pid)
{
PCB386 *retval = -1;
DWORD cpuflags;
PCB386 *head_ptr = sched_phead, *ptr;
ptr = head_ptr;

storeflags(&cpuflags);
stopints();

do  {
      if (ptr->processid == pid) {retval = ptr;break;};
      ptr = ptr ->next;
    }
while (ptr != head_ptr);
    
restoreflags(cpuflags);
return retval;
};

/*****************************************************************************
int sched_listprocess(PCB386 *process_buf,int items)
    process_buf = an array of PCB386 structures.
    items       = the maximum number of items process_buf can hold
    
return value: The total number of processes, if process_buf is NULL then
              the function simply returns the total number of processes.
              items must be non-zero 
-places the list of processes into a buffer*/
int sched_listprocess(PCB386 *process_buf, DWORD size_per_item, int items)
{
    DWORD cpuflags;
    int i = 0;
    PCB386 *head_ptr = sched_phead, *ptr;
    
    ptr = head_ptr;
    
    storeflags(&cpuflags);
    stopints();
    do
        {
                       
            if (process_buf!=0 && items!=0 )
                   { 
                   if (i < items)    
                   memcpy(&process_buf[i], ptr, size_per_item < sizeof(PCB386) ?
                                            size_per_item : sizeof(PCB386) );
                        else
                   break;
                   };
            ptr = ptr ->next; i++;                                            
        }
    while (ptr != head_ptr);
    
    restoreflags(cpuflags);
    
    return i;
};

//registers this scheduler extension to the device manager
void ps_scheduler_install()
{
 //create a scheduler extension, fill up required information
   memset(&ps_scheduler,0,sizeof(devmgr_scheduler_extension));
   ps_scheduler.hdr.size = sizeof(devmgr_scheduler_extension);
   ps_scheduler.hdr.type = DEVMGR_SCHEDULER_EXTENSION;
   strcpy(ps_scheduler.hdr.name,"default_sched");
   strcpy(ps_scheduler.hdr.description,"Default Round-Robin Scheduler");
   
   ps_scheduler.exthdr.attach  = sched_attach;
   ps_scheduler.ps_enqueue     = sched_enqueue;
   ps_scheduler.ps_dequeue     = sched_dequeue;
   ps_scheduler.scheduler      = scheduler;
   ps_scheduler.ps_gethead     = sched_gethead;
   ps_scheduler.ps_listprocess = sched_listprocess;
   ps_scheduler.ps_findprocess = sched_findprocess;
   //make interface available to the device manager
   ps_schedid = devmgr_register(&ps_scheduler);

   //update the current scheduler
   #ifdef DEBUG_STARTUP
      printf("process manager: Installing default scheduler (Simple Round-Robin)\n");
   #endif
   
   extension_override(devmgr_getdevice(ps_schedid),0);   
};
