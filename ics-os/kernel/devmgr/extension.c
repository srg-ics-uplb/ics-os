/*
  Name: extension.c
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 11/01/04 17:07
  Description: This is the extension manager, the extension manager enables
  the user and various user applications to customize various aspects of the
  operating system to their needs. It is also useful for testing out new
  OS algorithms and various other experiments.
  
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

/*Initializes the extension table, the extension table is an area of 
  memory where DEX keeps track of the modules which are currently active*/
void extension_init()
{
   int extension_table_size = sizeof(extension_info) * EXT_MAXEXTENSIONS; 
   extension_table = (extension_info*) malloc( extension_table_size );
   memset(extension_table,0,extension_table_size);
};



/*Override a module in memory and replace its interface with the interface
  of ext, the pointer to the previous interface is returned in prev*/
int extension_override(devmgr_generic *ext,devmgr_generic **prev)
{
	int retval = -1;

	
	DWORD cpuflags;
	while (extension_busy);
    extension_busy = 1;

	if (ext->type == DEVMGR_SCHEDULER_EXTENSION)
      {
      //get the current scheduler from the extension table
      devmgr_scheduler_extension *current_sched = (devmgr_scheduler_extension*)
                          extension_table[CURRENT_SCHEDULER].iface;
                          
      devmgr_scheduler_extension *new_sched = (devmgr_scheduler_extension*)ext;
      #ifdef DEBUG_EXTENSION
      printf("extension: changing schedulers..\n");
      #endif
      if (sizeof(devmgr_scheduler_extension)!=new_sched->hdr.size)
      printf("extension: warning sizes of scheduler interfaces not consistent!\n");
      
      if (current_sched!=0&&prev!=0)
                *prev = extension_table[CURRENT_SCHEDULER].iface;	
	        
            //Tell the current scheduler to get ready to be removed
            if (current_sched!=0)
            if (current_sched->exthdr.pre_remove!=0)
            current_sched->exthdr.pre_remove();           
                                         	         
	        //wait for the process manager to finish its task
	        sync_entercrit(&processmgr_busy);

	        storeflags(&cpuflags);
	        stopints();
  	        //Tell the current scheduler to remove itself if possible             
  	        
	        if (current_sched!=0)
	        if (current_sched->exthdr.remove!=0)
      	        current_sched->exthdr.remove();
      	        
            /*Tell the new scheduler to initialize itself or perform its
              own fixups*/
             #ifdef DEBUG_EXTENSION
             printf("extension: calling attach()\n");
             #endif

            if (new_sched->exthdr.attach!=0&&current_sched!=0)
            new_sched->exthdr.attach(current_sched);
            
            //replace the current scheduler
      		extension_table[CURRENT_SCHEDULER].deviceid = new_sched->hdr.id;
      		extension_table[CURRENT_SCHEDULER].iface = new_sched;
      		
            sync_leavecrit(&processmgr_busy);
      		//restore interrupts if it was enabled before and we're done!
      		restoreflags(cpuflags);
      		
	       

            retval = 1;
      #ifdef DEBUG_EXTENSION
      printf("extension: done!\n");
      #endif

      }
   else
   if (ext->type == DEVMGR_MALLOC_EXTENSION)
      {
          
          if (auxillary_malloc_base!=0) 
          {
              printf("extension: malloc functions may only be overriden once!\n");
              return -1;
          };
          
          devmgr_malloc_extension *current_malloc = (devmgr_malloc_extension*)
                              extension_table[CURRENT_MALLOC].iface;
          devmgr_malloc_extension *new_malloc = (devmgr_malloc_extension*)ext;
    
          if (sizeof(devmgr_malloc_extension)!=new_malloc->hdr.size)
          printf("extension: warning sizes of malloc interfaces not consistent!\n");
          
          storeflags(&cpuflags);
          stopints();
          
          auxillary_malloc_base = sbrk(0);
          
          extension_table[CURRENT_MALLOC].deviceid = ext->id;            
          extension_table[CURRENT_MALLOC].iface = ext;
          
          restoreflags(cpuflags);
          retval = 1;
      }
   else
   if (ext->type == DEVMGR_VMM_EXTENSION)
      {
          devmgr_vmm *current_vmm = (devmgr_vmm*) extension_table[CURRENT_VMM].iface;
          devmgr_vmm *new_vmm = (devmgr_vmm*)ext;
          
          if (current_vmm == 0)
          {
             storeflags(&cpuflags);
             stopints();
             extension_table[CURRENT_VMM].deviceid = ext->id;
             extension_table[CURRENT_VMM].iface = ext;
             restoreflags(cpuflags);
          };
          
      }
   else
      printf("extension: (Error) cannot identify extension type.\n");
      
   extension_busy = 0;
   return retval;
};

/*checks if a process is using a particular module*/
int extension_checkmodule(int deviceid)
{
    PCB386* ptr;
    int total,i;
    total = get_processlist(&ptr);
    
    for (i=0;i<total;i++)
    {
      if (ptr[i].context == deviceid) return 1;
    };
    
    free(ptr);
    return 0;
};

/*
    print the contents of the Extension table
*/

int extension_list()
{
   devmgr_generic *dev; 
   printf("Dex32 currently installed extensions.\n");
   printf("-------------------------------------\n");
   
   if (extension_table[CURRENT_SCHEDULER].iface!=0)
   {
   printf("SCHEDULER          :%-25s %-30s\n",
           extension_table[CURRENT_SCHEDULER].iface->name,
           extension_table[CURRENT_SCHEDULER].iface->description);
   };
   
   if (extension_table[CURRENT_SCHEDULER].iface!=0)
   { 
   printf("KMALLOC            :%-25s %-30s\n",
           extension_table[CURRENT_MALLOC].iface->name,
           extension_table[CURRENT_MALLOC].iface->description);
   };
   
   if (extension_table[CURRENT_VMM].iface!=0)
   {
   printf("VIRTUAL MEMORY MGR.:%-25s %-30s\n",
           extension_table[CURRENT_VMM].iface->name,
           extension_table[CURRENT_VMM].iface->description);
   };
   
   printf("-------------------------------------\n");
};
