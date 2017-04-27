/*
  Name: bridges.c
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 11/01/04 18:04
  Description: This module brdiges calls between extensible modules. This module was
  largely influenced by the AOP (Aspect Oriented Programming) Joint-Point concept. It is
  now possible to effectively monitor inter-module communication by simply monitoring
  the bridges.The only disadvantage of bridges is that it will to some degree reduce
  system efficiency because of the added overhead, by how much I am still not certain.
  Ideally all intermodule calls should go through the bridge, unfortunately, this 
  is still not completely implemented as of yet.
  
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

//A bridge to the current scheduler
PCB386 *bridges_ps_scheduler(PCB386 *prev)
{
PCB386 *returnval;
devmgr_scheduler_extension *sched = extension_table[CURRENT_SCHEDULER].iface;
sched->exthdr.busywait = 1;
returnval = sched->scheduler(prev);
sched->exthdr.busywait = 0;
return returnval;
};

PCB386 *bridges_ps_findprocess(int pid)
{
PCB386 *returnval;
devmgr_scheduler_extension *sched = extension_table[CURRENT_SCHEDULER].iface;
sched->exthdr.busywait = 1;
returnval = sched->ps_findprocess(pid);
sched->exthdr.busywait = 0;
return returnval;
};

DWORD bridges_link(devmgr_generic *dev, void **function,
                DWORD p1,DWORD p2,DWORD p3,DWORD p4,DWORD p5,DWORD p6)
{
    DWORD (*module_function)(DWORD,DWORD,DWORD,DWORD,DWORD,DWORD)=0;
    DWORD retval = 0;
    int function_number = ((DWORD)function - (DWORD)dev - sizeof(devmgr_generic)) / 4;
    int last_context = devmgr_getcontext();
    #ifdef DEBUG_BRIDGE
    printf("bridge to %d called fxn id %d.\n", dev->id,function_number);
    #endif
    
    if (dev==0) return -1;
    /*Set current module ID and function ID so that the module knows itself
      as well as for debuggin purposes*/
    devmgr_setcontext(dev->id);
    
    devmgr_setfunction(function_number);
    
    module_function = *function;
    
    //Make sure we're calling something
    if (module_function == 0) return -1;
    
    //call the module's function
    retval = module_function(p1,p2,p3,p4,p5,p6);
    
    
    devmgr_setcontext(last_context);
    
    return retval;
};

void bridges_init()
{
    bridges_call = bridges_link;
};


