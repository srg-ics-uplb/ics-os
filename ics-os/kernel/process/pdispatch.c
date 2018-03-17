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


/* called by modules or applications to add a process with the image of the
 * executable file as parameters
 * 
 */
int addmodule(char *name, char *image, char *loadaddress, int mode, char *parameter,
                char *workdir, int parent){

   //Make sure no one else is in its critical section by disabling interrupts
   DWORD flags;
   storeflags(&flags);
   stopints();
   
   if (pd_head == 0){               //create a new head node since it does not exist                                           
      pd_head=(createp_queue*)malloc(sizeof(createp_queue));
      pd_head->next=0;
   }
   else{                            //add a new node to the head
      pd_head->next=pd_head;
      pd_head=(createp_queue*)malloc(sizeof(createp_queue));
   };

   //At this point the new node has been created to 
   //set the values of the fields
   pd_head->handle=(DWORD)pd_head;  //Set the handle to the adddress, unique to node
   pd_head->image=image;            //Set the program image

   pd_head->type = NEW_MODULE;      //NEW_MODULE means PCB is not a copy of parent

   if (parameter != 0)              //Set the command line arguments
      strcpy(pd_head->parameter,parameter);
       
   pd_head->mode=mode;              //Set the mode
   pd_head->loadaddress = loadaddress; //Set the load address
   pd_head->dispatched = 0;         //Set to 0 since no process for this node has been created
   pd_head->parent = parent;        //Set the parent
   strcpy(pd_head->name, name);     //Set the name
   strcpy(pd_head->workdir, workdir);  //Set the working dir
       
    
   restoreflags(flags);             //We're done adding the new node to pd_head
   return pd_head->handle;          //return the handle
};

/*
 * Called in user_fork() from kernel/console/console.c
 * Creates a node that is added to pd_head for a forked process
 */
int pd_forkmodule(int parent){
   DWORD flags;
   createp_queue *tmp;

   storeflags(&flags);
   stopints();


   //Create a createp_queue node for the child
   if(pd_head==0){
      pd_head=(createp_queue*)malloc(sizeof(createp_queue));
      pd_head->next=0;
   }else{
      pd_head->next=pd_head;
      pd_head=(createp_queue*)malloc(sizeof(createp_queue));
      //tmp=pd_head;
      //pd_head=(createp_queue*)malloc(sizeof(createp_queue));
      //pd_head->next=tmp;
   };
       
   pd_head->handle      = (DWORD)pd_head;
   pd_head->type        = FORK_MODULE;          //FORK_MODULE means create a copy of parent PCB
   pd_head->image       = 0;                    //no image since fork module
   pd_head->dispatched  = 0;                    //No process has been dispatched yet for this node
   pd_head->parent      = parent;               //Set the parent
   
   restoreflags(flags);                         //restore the flags
   return pd_head->handle;
};

//Determines if a process has already been dispatched for a given createp_queue node.
//returns the process id of the process created
int pd_dispatched(int handle){
   createp_queue *ptr=(createp_queue*)handle;
   int retval=0;
 
   //make sure no other process is accessing the pd_head 
   while (pd_busy)
      ;
   
   //we're in the critical section, block other process from accessing pd_head
   pd_busy = 1;  

   if (ptr->dispatched){      //Is the process for this node has been dispatched
      if (ptr->processid == 0) 
         retval = -1; /*pid of 0? Error loading executable?(only the kernel has a pid of 0) */
      else
         retval=ptr->processid;  //Return the process id of the process that was created for this node
   };
   
   //give chance to others to access the pd_head 
   pd_busy = 0;
   return retval;
};

/**
 *Check to see if a process for a node, identified 
 *by handle, has been dispatched. Frees the node if yes.
 *Calls pd_dispatched()
 */
int pd_ok(int handle){

   //get a pointer to the node
   createp_queue *ptr=(createp_queue*)handle;

   //Has the process been dispatched for the node?
   int retval = pd_dispatched(handle);

   //make sure no other process is accessing the pd_head 
   while (pd_busy)
      ;
   
   //we're in the critical section, block other process from accessing pd_head
   pd_busy = 1;

   if (retval !=0 && retval != -1)  //Ok a process has been dispatched for the node  
      free(ptr);                    //deallocate the space for the node in the queue
 
   //give chance to others to access the pd_head 
   pd_busy = 0;   

   return retval;
};

//The process dispatcher is a kernel service/thread that handles process
//creation requests, it calls the module loader. A unique aspect of this
//process dispatcher is that it is the only process that is capable of
//turning paging off and living to tell the tale. This is because its
//stack segment is located at a 1-to-1 virtual to physical memory area. Although
//It may not be able to use the kernel heap when it disables paging.
//called in dex32_startup() from kernel/kernel32.c. It is the last task of dex_init()
void process_dispatcher(){
   createp_queue *ptr;
   DWORD sender,message,data;
   DWORD pid;
  
   while (1){
      if (pd_head != 0){      //while there is an entry in process dispatcher queue, pd_head
         DWORD flag,pid;

         PCB386 *parent = ps_findprocess(pd_head->parent);     //find the parent process
                
         //use the working directory of the requesting process
         //so that file operations used by the module loader like
         //loadDLL works correctly
         current_process->workdir = parent->workdir;
                
         //Check the type of the node which determines how the process will be created.
         if (pd_head->type == FORK_MODULE){        //a FORK_MODULE. Perform a forkprocess()
            #ifdef DEBUG_FORK
               printf("process dispatcher received fork request\n");
            #endif            

            pd_head->dispatched=0;  //not yet dispatched
            pd_head->processid = forkprocess(parent);
            pd_head->dispatched=1;  //ok a process has been created                  

            #ifdef DEBUG_FORK
               printf("fork request done.\n");
            #endif
         }else{               //a NEW_MODULE. Perform a createprocess(),implicitly called in module loaders     

            pd_head->dispatched=0;  //not yet dispatched
            pd_head->processid = dex32_loader(pd_head->name, pd_head->image, pd_head->loadaddress,
                                    pd_head->mode, pd_head->parameter, pd_head->workdir, parent);
            pd_head->dispatched=1;  //ok a process has been created

            ptr=pd_head;
         };  
                 
         pd_head=pd_head->next;
                
      };

      //check for a shutdown message
      if (getmessage(&sender, &message, &data)!=0){
         if (message == MES_SHUTDOWN){
            int total = 0, i;
            PCB386 *ptr;        
            printf("Kernel received MES_SHUTDOWN.\n");
                             
            printf("Sending all processes the SHUTDOWN MESSAGE..\n");
            forceflush = 1;
            broadcastmessage(0, MES_SHUTDOWN,0);
            total = get_processlist(&ptr);
            printf("Killing all processes\n");
            for (i=0; i < total; i++)
               if (ptr[i].processid!=0){
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
               
   
