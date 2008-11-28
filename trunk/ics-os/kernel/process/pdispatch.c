/*
  Name: DEX process dispatcher
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 23/01/04 04:53
  Description: This module is responsible for mapping processes from files, calling
  the appropriate moduleloader and starting them.
  
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


//called by modules or applications to add a process with the image of the
//executable file as parameters
int addmodule(char *name,char *image,char *loadaddress,int mode,char *parameter,
                char *workdir,int parent)
  {
    //Make sure no one else is in its critical section
    DWORD flags;
    storeflags(&flags);
    stopints();
   
     if (pd_head==0)
       {
       pd_head=(createp_queue*)malloc(sizeof(createp_queue));
       pd_head->next=0;
       }
         else
       {
       pd_head->next=pd_head;
       pd_head=(createp_queue*)malloc(sizeof(createp_queue));
       };
       pd_head->handle=(DWORD)pd_head;
       pd_head->image=image;
       pd_head->type = NEW_MODULE;
       if (parameter!=0)
       strcpy(pd_head->parameter,parameter);
       
       pd_head->mode=mode;
       pd_head->loadaddress = loadaddress;
       pd_head->dispatched=0;
       pd_head->parent=parent;
       strcpy(pd_head->name,name);
       strcpy(pd_head->workdir,workdir);
       
    
       restoreflags(flags);
       return pd_head->handle;
  ;};

//called by modules or applications to fork this current process  
int pd_forkmodule(int parent)
{
    DWORD flags;
    storeflags(&flags);
    stopints();
       if (pd_head==0)
       {
              pd_head=(createp_queue*)malloc(sizeof(createp_queue));
              pd_head->next=0;
       }
         else
       {
              pd_head->next=pd_head;
              pd_head=(createp_queue*)malloc(sizeof(createp_queue));
       };
       
       pd_head->handle=(DWORD)pd_head;
       pd_head->image=0;
       pd_head->type = FORK_MODULE;
       pd_head->dispatched=0;
       pd_head->parent=parent;
       
    restoreflags(flags);  
    return pd_head->handle;
};

//Determines if a process has already been dispatched.
int pd_dispatched(int handle)
{
createp_queue *ptr=(createp_queue*)handle;
int retval=0;
  
while (pd_busy);
pd_busy = 1;  
if (ptr->dispatched)
     {
      if (ptr->processid==0) 
      retval=-1; /*pid of 0? Error loading executable?(only the kernel has a pid of 0) */
           else
      retval=ptr->processid;

     };
     
pd_busy = 0;
return retval;
};

//Determines if a process has already been dispatched, and frees up 
//memory used by the createp_queue data structure
int pd_ok(int handle)
 {
  createp_queue *ptr=(createp_queue*)handle;
  int retval = pd_dispatched(handle);
 
  while (pd_busy);
  pd_busy = 1;
  if (retval!=0 && retval !=-1) free(ptr);
  pd_busy = 0;   
  return retval;
 };

//The process dispatcher is a kernel service/thread that handles process
//creation requests, it calls the module loader. A unique aspect of this
//process dispatcher is that it is the only process that is capable of
//turning paging off and living to tell the tale. This is because its
//stack segment is located at a 1-to-1 virtual to physical memory area. Although
//It may not be able to use the kernel heap when it disables paging.
void process_dispatcher()
  {
    createp_queue *ptr;
    DWORD sender,message,data;
    DWORD pid;
       while (1)
            {
               if (pd_head!=0)
               {
                DWORD flag,pid;
                PCB386 *parent = ps_findprocess(pd_head->parent);
               
                
                //use the working directory of the requesting process
                //so that file operations used by the module loader like
                //loadDLL works correctly
                current_process->workdir = parent->workdir;
                
                //A Fork process command was requested
                if (pd_head->type == FORK_MODULE)
                  {
                        #ifdef DEBUG_FORK
                        printf("process dispatcher received fork request\n");
                        #endif            
                        pd_head->dispatched=0;
                        pd_head->processid = forkprocess(parent);
                        pd_head->dispatched=1;                  
                        #ifdef DEBUG_FORK
                        printf("fork request done.\n");
                        #endif
                  }
                     else
                 //A Normal Createprocess command was requested    
                 {
                        pd_head->dispatched=0;
                        pd_head->processid = dex32_loader(pd_head->name,pd_head->image,pd_head->loadaddress,
                                    pd_head->mode,pd_head->parameter,pd_head->workdir,parent);
                        pd_head->dispatched=1;
                        ptr=pd_head;
                 };  
                 
                 pd_head=pd_head->next;
                
                };
                
                if (getmessage(&sender,&message,&data)!=0)
                   {
                       if (message == MES_SHUTDOWN)
                          {
                             int total = 0, i;
                             PCB386 *ptr;        
                             printf("Kernel received MES_SHUTDOWN.\n");
                             
                             printf("Sending all processes the SHUTDOWN MESSAGE..\n");
                             forceflush = 1;
                             broadcastmessage(0,MES_SHUTDOWN,0);
                             total = get_processlist(&ptr);
                             printf("Killing all processes\n");
                             for (i=0; i < total; i++)
                                   if (ptr[i].processid!=0)
                                   {
                                       if (ptr[i].accesslevel == ACCESS_SYS)
                                          dex32_killkthread(ptr[i].processid);
                                       else
                                       ps_user_kill(ptr[i].processid);
                                   };
                            
                             printf("System halted. You may now turn off your computer.\n");
                             while (1);
                          };
                   };
             };   
   };
               
   
