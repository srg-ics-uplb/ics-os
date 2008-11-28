/*
  Name: Exception handling module
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 13/03/04 06:30
  Description: This module provides exception handlers for the operating system. There are
  exception handlers for GPFs, page faults and divide by zero. The page fault handler also
  handles demand loading requests.
  
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

void GPFhandler(DWORD address)
  {
  char temp[255];
    stopints();
    
    exc_showdump(address,GENERAL_PROTECTION_FAULT,0);
    exc_recover();
        
    while (1) {};
    startints();

  };
  
void exc_invalidtss(DWORD address)
  {
  char temp[255];
    stopints();
    
    exc_showdump(address,INVALID_TSS,0);
    exc_recover();
        
    while (1) {};
    startints();

  };
void divide_error(DWORD address)
  {
    stopints();
    exc_showdump(address,DIVIDE_ERROR,0);
    exc_recover();
    while (1) {};
    startints();

  };
  

//show the contents of the EFLAGS register
void exc_dumpflags(DEX32_DDL_INFO *output, DWORD flags)
{
    int ID,VIP,VIF,AC,VM,RF,NT,IOPL,OF;
    int DF,IF,TF,SF,ZF,AF,PF,CF;
    
    //Perform a bit by bit comparitson
    CF   =  (flags & 1) ?   1 : 0;
    PF   =  (flags & 4) ?   1 : 0;
    AF   =  (flags & 16) ?  1 : 0;
    ZF   =  (flags & 64) ?  1 : 0;
    SF   =  (flags & 128) ? 1 : 0;
    TF   =  (flags & 256) ? 1 : 0;
    IF   =  (flags & 512) ? 1 : 0;
    DF   =  (flags & 1024) ? 1 : 0;
    OF   =  (flags & 2048) ? 1 : 0;
    IOPL =  (flags & 0x3000) >> 12;
    NT   =  (flags & 0x4000) ? 1 : 0;
    RF   =  (flags & 0x10000) ? 1 : 0;
    VM   =  (flags & 0x20000) ? 1 : 0;
    AC   =  (flags & 0x40000) ? 1 : 0;
    
    DDLprintf(output,
    "CF=%d PF=%d AF=%d ZF=%d SF=%d TF=%d IF=%d DF=%d OF=%d IO=%d NT=%d RF=%d VM=%d AC=%d\n",
    CF,PF,AF,ZF,SF,TF,IF,DF,OF,IOPL,NT,RF,VM,AC);

};



//show the contents of the CPU registers and other information
void exc_showdump(DWORD location,int type,DWORD pf_info)
{
   int ret;
   char fault_type[25];
   DWORD pageentry,direntry,ktopheap = knext;
   DEX32_DDL_INFO *beforeout,*showdumpout;


   strcpy(fault_type,"Exception -");

   //convert the value given in type to string  
   if (location >= 0xFFFF0000 && location <=0xFFFFFFF0) strcat(fault_type,"unresolved import error");
      else
   if (type == PAGE_FAULT) sprintf(fault_type,"Page fault (0x%x)",pf_info);
      else
   if (type == GENERAL_PROTECTION_FAULT) strcat(fault_type,"General Protection fault");
      else
   if (type == DIVIDE_ERROR) strcat(fault_type,"Divide by zero ");
      else
   if (type == INVALID_TSS) strcat(fault_type,"Invalid Task State Segment");
      else
   strcat(fault_type,"unknown fault");   
   
   #ifdef FULLSCREENERROR   
   direntry=getpagetablephys(location, current_process->pagedirloc);
   pageentry=getphys(location,current_process->pagedirloc);
   showdumpout = Dex32CreateDDL();
   beforeout = Dex32SetActiveDDL(showdumpout);
   
   Dex32Clear(showdumpout);
   Dex32SetTextBackground(showdumpout,RED);
   Dex32SetTextColor(showdumpout,WHITE);
   DDLprintf(&showdumpout,"%-79s\n",fault_type);
   Dex32SetTextBackground(showdumpout,BLACK);
   DDLprintf(&showdumpout,"Faulting process                       :%s\n",current_process->name);
   DDLprintf(&showdumpout,"Process ID                             :%d\n",current_process->processid);
   
   DDLprintf(&showdumpout,"\n============ <Memory Access Information>=================\n");
   DDLprintf(&showdumpout,"Tried to access invalid memory location: 0x%x\n",location);

   DDLprintf(&showdumpout,"Page directory entry at that address is: 0x%x\n",direntry);

   DDLprintf(&showdumpout,"Page table entry at that address is    : 0x%x\n",pageentry);
   DDLprintf(&showdumpout,"Address of faulting instruction is     : 0x%x\n",current_process->regs.EIP);
   DDLprintf(&showdumpout,"Kernel Pagedirectory : 0x%x  Process Pagedirectory: 0x%x",pagedir1,current_process->regs.CR3);
   
   DDLprintf(&showdumpout,"\n============ <Register values at time of fault>==========\n");

   DDLprintf(&showdumpout,"EAX=0x%x EBX=0x%x ECX=0x%x EDX=0x%x ",current_process->regs.EAX,
   current_process->regs.EBX,current_process->regs.ECX,current_process->regs.EDX);

   DDLprintf(&showdumpout,"EBP=0x%x EDI=0x%x \nESI=0x%x ESP=0x%x ",
   current_process->regs.EBP,current_process->regs.EDI,
   current_process->regs.ESI,current_process->regs.ESP);
   
   DDLprintf(&showdumpout,"CS=0x%x DS=0x%x ES=0x%x SS=0x%x ",
   current_process->regs.CS,current_process->regs.DS,current_process->regs.ES,
   current_process->regs.SS);
   
   DDLprintf(&showdumpout,"FS=0x%x GS=0x%x \n",current_process->regs.FS,current_process->regs.GS);
   
   exc_dumpflags(&showdumpout,current_process->regs.EFLAGS);
   
   
    
   DDLprintf(&showdumpout,"\n\nAPI call information\n");
   DDLprintf(&showdumpout,"=========================================================\n"); 
   DDLprintf(&showdumpout,"Process Top of Heap location: 0x%x\n",(DWORD)current_process->knext);
   DDLprintf(&showdumpout,"Kernel Top of Heap location: 0x%x\n",ktopheap);
   DDLprintf(&showdumpout,"last system calls:(1) : 0x%x ,(2-last): 0x%x\n",
   current_process->cursyscall[0],current_process->cursyscall[1]);
   DDLprintf(&showdumpout,"context information:  device # %d(%s), function # %d(%s)\n",
             current_process->context,
             devmgr_getname(current_process->context),
             current_process->function,
             get_function_name(current_process->context,current_process->function));



   if (current_process->op_success==1) DDLprintf(&showdumpout,"syscall terminated normally\n");
      else
      {
         DDLprintf(&showdumpout,"Fault occured during system call.\n");
      };
      
   DDLprintf(&showdumpout,"Press any key to continue...");
   
   disable_taskswitching();
   startints();
   kb_pause();

   //return to the previous screen
   Dex32SetActiveDDL(beforeout);
  
   #else
      printf("%s\n",fault_type);
      printf("faulting process                       :%s\n",current_process->name);
      printf("Tried to access invalid memory location: 0x%x\n",location);
      printf("Fault occured during system call. This might be a bug.\n");
      printf("System halted for protection purposes\n.");
      printf("Page directory entry at that address is: 0x%x\n",ret);
      printf("Address of faulting instruction is     : 0x%x\n",current_process->regs.EIP);
      while(1){};
   #endif
   
   stopints();

   enable_taskswitching();
   
};

// the core function that handles page faults, it is also
// dirctly linked to the virtual memory manager of the operating system
DWORD pagefaulthandler(DWORD location,DWORD fault_info)
  {
   DWORD ret,mm,i;
   stopints();
   pfoccured=1;//set the pfoccured register of the task scheduler

   mm=getphys(location,current_process->pagedirloc);
   
   if (mm&PG_DEMANDLOAD)
         {
             //A page marked as demand paged has been called so
             //we allocate a physical frame to this page
             DWORD pg,pageadr=(DWORD)mempop();

           //  printf("demand paged %s..",itoa(location,temp,16));
             if (pageadr==0) //we're out of physical frames, find a way to get one
             pageadr=obtainpage(); //call the VMM to get a page

             pg=(DWORD)getvirtaddress((DWORD)current_process->pagedirloc);

             maplineartophysical2(pg,location,
             pageadr,PG_PRESENT | PG_USER | PG_WR);
   
             pg=(DWORD)getvirtaddress(pageadr);
             memset(pg,0,0x1000);
                
             //Finished allocation, let the task scheduler take over
             setattb(PF_TSS,0x89); //reset the TSS attribute
             taskswitch();
        ;};
   
   // a copy-on-write page has been written to, we therefore duplicate this page
   // so that forked processes have their own unique set of data.     
   if (mm&PG_COPYWRITE) 
        {
                DWORD destpg,pdirpg;
                DWORD pageadr= (DWORD) mempop(); //allocate new memory
                char pagebuf[0x1000];
                if (pageadr==0) //we're out of physical frames, find a way to get one
                //call the VMM to get a page, "assume" it works
                pageadr=obtainpage(); 

                                
                //obtain a virtual memory address for this physical address
                destpg=(DWORD)getvirtaddress(pageadr); 
                                                       
                //copy the content of the copy-on-write page to the new page         
                memcpy(destpg, location&0xFFFFF000 ,0x1000);
                
                //obtain a virtual address for the page directory
                pdirpg=(DWORD)getvirtaddress((DWORD)current_process->pagedirloc);
                
                //update the page directory of the process to reflect this change
                maplineartophysical2(pdirpg,location,
                pageadr,PG_PRESENT | PG_USER | PG_WR);

                #ifdef DEBUG_FORK                 
                printf("COPY ON WRITE DETECTED.\n");
                #endif
                
                setattb(PF_TSS,0x89); //reset the TSS attribute
                taskswitch();                
        };
   //show register dump and other important information
   exc_showdump(location,PAGE_FAULT,mm);        
   //Try to recover from the fault
   exc_recover();
    
   while (1) {};
   startints();
  };

  
  //This function is called when DEX is trying to recover from a fault
  void exc_recover()
  {
     if (current_process->processid==0)
    {
     printf("dex32_kernel: kernel mode page fault.\n");
     printf("recovery mode active. system may become unstable.\n");
     printf("System Halted\n");
     while (1);
    }
     else
    {
     printf("dex32_kernel: user mode page fault.\n");
     printf("recovery mode active. system may become unstable.\n");
     printf("shutting down application..\n");
     //remove process from the process queue
     exit(0);
     startints();
    };
  };

