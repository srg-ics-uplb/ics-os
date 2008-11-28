/*
  Name: DEX 32-bit binary executable format .B32
  Copyright: 
  Author: Joseph Emmanuel Dl Dayo
  Date: 14/03/04 18:36
  Description: This executable format provides a "quick and dirty way" to create
  executable files using nasm. Binaries of this type must be statically compiled
  to run at 0x400000.
  
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


#define magic 0xDA40
#define B32_STACKSIZE 0xFFFF
#define B32_HEAPSIZE  0xFFFF

typedef struct  {
DWORD zmagic;
DWORD entrypoint;
DWORD symbol_table;
DWORD image_size;
} B32_fileheader;

typedef struct  {
char func_name[256];
DWORD address;
DWORD type;
} B32_symboltable;


//loads a user mode pe module and creates a process to start it
int b32_loadusermodule(
        char *module_name, //the name of the module
        char *b32_image,  //the location of the pe image
        DWORD base, //the desired base address to load the image so
                    //that dex can perform the necessary relocations
        int mode,   //Determines what kind of PE to load, USEREXE, USERDLL, SYSDLL or SYSEXE
        char *p,    //the parameters
        char *workdir, //The working directory of the program
        PCB386 *parent //the parent PCB for EXEs, the parent pagedirectory for DLLs
        ) //location of image to load
{
	B32_fileheader *fhdr=(B32_fileheader*) b32_image;
	
	if (fhdr->zmagic==0xDA40)
	{
	DWORD *pagedir,*pg;
  	DWORD heapres=0,stackres=0;
  	DWORD *stackloc;
  	DWORD flags,pages,ret;
  	void (*entrypoint)(int,char**)=0;
  	process_mem *memptr=0;
	
	#ifdef DEBUG_B32
	printf("dex32 standard flat binary loader\n");
 	#endif
 			
	//allocate heaps and stacks
   	pagedir=(DWORD*)mempop(); //obtain a physical address from the memory manager
    pg=(DWORD*)getvirtaddress((DWORD)pagedir); //convert to a virtual address so that
                                                          //we could use it here without disabling
                                                          //the paging mechanism of the 386

               //initialize the new pagedirectory
               memset(pg,0,0x1000);
        
               stackloc=(DWORD*)dex32_commitblock((DWORD)userstackloc-B32_STACKSIZE,
               B32_STACKSIZE,&pages,
               pagedir,PG_WR | PG_USER | PG_PRESENT);
               
               addmemusage(&memptr,stackloc,pages);

               //allocate the "reserve" user stack
               //this pages don't get a physical memory until it actually gets
               //accessed

               stackres=dex32_reserveblock((DWORD)stackloc-DEFAULT_STACKSIZE,
               DEFAULT_STACKSIZE,&pages,pagedir,PG_WR);

               //store the allocation information so that it could be linked to
               //the process PCB and then be able to free up the memory when app terminates
               addmemusage(&memptr,stackres,pages);
               
               //calculate the position of the ESP pointer
               stackloc = userstackloc - 4 ;

               //allocate "commited" user heap
               //unlike the stack, this one goes up
               dex32_commitblock((DWORD)userheap, B32_HEAPSIZE, &pages,
                        pagedir,PG_WR | PG_USER | PG_PRESENT);

               //take note of this, so that it wouldn't cause a memory leak later
               addmemusage(&memptr,userheap,pages);
               
               //calculate the position of the reserved heap
               heapres=userheap+(pages*0x1000);

               //allocate "reserved" user heap
               dex32_reserveblock((DWORD)heapres,
                      DEFAULT_HEAPSIZE,&pages,pagedir,PG_WR );

               addmemusage(&memptr,heapres,pages);


		       //copy image to memory
		
		       dex32_commitblock((DWORD)base,fhdr->image_size,&pages,
               pagedir,PG_WR | PG_USER | PG_PRESENT);
        
               addmemusage(&memptr,base,pages);            	
  		       memcpy(base,b32_image,fhdr->image_size);
     
               dex32_stopints(&flags);
               
                   entrypoint=fhdr->entrypoint;
                   ret=createprocess(entrypoint,module_name,pagedir,memptr,stackloc,
                   0x2000,SYSCALL_STACK,signal,p,workdir,parent);
                   dex32_freeuserpagetable(pagedir1);             
                   
               dex32_restoreints(flags);
            
		
	     return ret;
	};
	
	return 0;
};
