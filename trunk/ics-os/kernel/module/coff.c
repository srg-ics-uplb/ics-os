/*
  Name: 32-bit COFF format executable loader
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 13/03/04 06:31
  Description: this module specifically loads 32-bit COFF format executables and libraries.
  Implementation is not complete yet.
  
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

//magic values which identify a type of executable
#define I386MAGIC  0x14C
#define ZMAGIC 0x10B

/*=============================================================*/
/*This defines the attribute bits which identify a section type*/
/*=============================================================*/

/*If set, indicates that this section contains only executable code.*/ 
#define  STYP_TEXT 0x0020 

/*If set, indicates that this section contains only initialized data.*/ 
#define  STYP_DATA 0x0040 

/*If set, indicates that this section defines uninitialized data, and has no data stored in the coff file for it.*/ 
#define  STYP_BSS 0x0080 


/*==========================================*/
/*defines the flags found in the File header*/
/*==========================================*/

/*If set, there is no relocation information in this file. This is usually clear for objects and set for executables. */
#define  F_RELFLG 0x0001
 
/*If set, all unresolved symbols have been resolved and the file may be considered executable. */
#define  F_EXEC 0x0002 

/*If set, all line number information has been removed from the file (or was never added in the first place). */
#define  F_LNNO 0x0004 

/* If set, all the local symbols have been removed from the file (or were never added in the first place). */
#define  F_LSYMS 0x0008 

/*Indicates that the file is 32-bit little endian*/ 
#define  F_AR32WR 0x0100

/*=========================================*/
/*defines the type of relocation to perform*/
/*=========================================*/

/*Relocate a 32-bit absolute reference*/
#define RELOC_ADDR32 6 

/*Relocate a 32-bit relative reference*/
#define RELOC_REL32 20 


/*=========================================*/
/*Defines the sizes for the HEAP and STACKS*/
/*=========================================*/


//Defines the File Header for a coff file, this was lifted from the
//DJGPP coff file specification

typedef struct __attribute__((packed)) {
  unsigned short f_magic;         /* magic number             */
  unsigned short f_nscns;         /* number of sections       */
  unsigned long  f_timdat;        /* time & date stamp        */
  unsigned long  f_symptr;        /* file pointer to symtab   */
  unsigned long  f_nsyms;         /* number of symtab entries */
  unsigned short f_opthdr;        /* sizeof(optional hdr)     */
  unsigned short f_flags;         /* flags                    */
} FILHDR;

//Defines the optional header for a coff file, this was liftd from the
//DJGPP coff file specification

typedef struct __attribute__((packed)) {
  unsigned short magic;          /* type of file                         */
  unsigned short vstamp;         /* version stamp                        */
  unsigned long  tsize;          /* text size in bytes, padded to FW bdry*/
  unsigned long  dsize;          /* initialized data    "  "             */
  unsigned long  bsize;          /* uninitialized data  "  "             */
  unsigned long  entry;          /* entry pt.                            */
  unsigned long  text_start;     /* base of text used for this file      */
  unsigned long  data_start;     /* base of data used for this file      */
} AOUTHDR;

typedef struct __attribute__((packed)) {
  unsigned long  r_vaddr;   /* address of relocation      */
  unsigned long  r_symndx;  /* symbol we're adjusting for */
  unsigned short r_type;    /* type of relocation         */
} RELOC;

//Defines a COFF section header
typedef struct __attribute__((packed)) {
  char           s_name[8];  /* section name                     */
  unsigned long  s_paddr;    /* physical address, aliased s_nlib */
  unsigned long  s_vaddr;    /* virtual address                  */
  unsigned long  s_size;     /* section size                     */
  unsigned long  s_scnptr;   /* file ptr to raw data for section */
  unsigned long  s_relptr;   /* file ptr to relocation           */
  unsigned long  s_lnnoptr;  /* file ptr to line numbers         */
  unsigned short s_nreloc;   /* number of relocation entries     */
  unsigned short s_nlnno;    /* number of line number entries    */
  unsigned long  s_flags;    /* flags                            */
} SCNHDR;

typedef struct __attribute__((packed)) {
  union {
    char e_name[8];
    struct {
      unsigned long e_zeroes;
      unsigned long e_offset;
    } e;
  } e;
  unsigned long e_value;
  short e_scnum;
  unsigned short e_type;
  unsigned char e_sclass;
  unsigned char e_numaux;
} SYMENT;




int coff_loadusermodule(
        char *module_name, //the name of the module
        char *coff_image,  //the location of the pe image
        DWORD base, //the desired base address to load the image so
                    //that dex can perform the necessary relocations
        int mode,   //Determines what kind of executable to load
        char *p,    //the parameters
        char *workdir,
        PCB386 *parent //the parent PCB for EXEs, the parent pagedirectory for DLLs
        ) //location of image to load
{
char temp[255],temp2[255];
FILHDR  *fhdr = (FILHDR*) coff_image;

 //a linked list used to keep track of the virtual addresses used
 //by this process so that it could be freed when the process
 //terminates
 process_mem *tmpr,*memptr;
 
 
//validate if this file is a coff format executable
#ifdef DEBUG_COFF
printf("dex32_coff_loader: started\n");
printf("dex32_coff_loader: magic number obtained [%s]\n",itoa(fhdr->f_magic,temp,16));
#endif

if (fhdr->f_magic==I386MAGIC)
  {
  	int i;
  	AOUTHDR *aouthdr = (AOUTHDR*)(coff_image + sizeof(FILHDR));
        SCNHDR  *section = (SCNHDR*)((DWORD)coff_image + sizeof(FILHDR)+sizeof(AOUTHDR));
        SYMENT  *symbols = fhdr->f_symptr;
        int totalsymbols = fhdr->f_nsyms;
        
  	DWORD *pagedir,*pg;
  	DWORD heapres=0,stackres=0;
  	DWORD *stackloc;
  	DWORD flags,pages,ret;
  	int totalsections = fhdr->f_nscns;
  	
        void (*entrypoint)(int,char**)=0;
        
	#ifdef DEBUG_COFF
		printf("dex32_coff_loader: magic number 2 obtained [%s]\n",itoa(aouthdr->magic,temp,16));
	#endif
	
	if (aouthdr->magic!=ZMAGIC) return 0; //check again
	printf("dex32_coff_loader: loading executable..\n");

	#ifdef DEBUG_COFF
		printf("dex32_coff_loader: entry point at %sH\n",itoa(aouthdr->entry,temp,16));
		printf("dex32_coff_loader: Total sections %d\n",totalsections);
	#endif

	entrypoint=(void*) aouthdr->entry;
	
	if (fhdr->f_flags&F_RELFLG)
	   {
	   	printf("dex32_coff_loader: Relocations have been stripped of the file\n");
	   };
	
	
	memptr=(process_mem*)malloc(sizeof(process_mem));
   	tmpr=memptr;
   	
   	pagedir=(DWORD*)mempop(); //obtain a physical address from the memory manager
        pg=(DWORD*)getvirtaddress((DWORD)pagedir); //convert to a virtual address so that
                                                          //we could use it here without disabling
                                                          //the paging mechanism of the 386

         //initialize the new pagedirectory
         memset(pg,0,0x1000);
        
         stackloc=(DWORD*)dex32_commitblock((DWORD)userstackloc-1000,
               1000,&pages,
               pagedir,PG_WR | PG_USER | PG_PRESENT);

               //take note of this so that we could free it later
               tmpr->vaddr=(DWORD)stackloc;
               tmpr->pages=pages;
               tmpr->next=(process_mem*)malloc(sizeof(process_mem));
               tmpr=tmpr->next;
               tmpr->vaddr=0;tmpr->next=0;


               //allocate the "reserve" user stack
               //this pages don't get a physical memory until it actually gets
               //accessed

               stackres=dex32_reserveblock((DWORD)stackloc-DEFAULT_STACKSIZE,
               DEFAULT_STACKSIZE,&pages,pagedir,PG_WR);

               //store the allocation information so that it could be linked to
               //the process PCB and then be able to free up the memory when app terminates
               tmpr->vaddr=(DWORD)stackres;
               tmpr->pages=pages;

               //create another node in the linked list
               tmpr->next=(process_mem*)malloc(sizeof(process_mem));
               tmpr=tmpr->next;
               tmpr->vaddr=0;tmpr->next=0;

               //calculate the position of the ESP pointer
               stackloc = userstackloc - 4 ;

               //allocate "commited" user heap
               //unlike the stack, this one goes up
               dex32_commitblock((DWORD)userheap,1000,&pages,
                        pagedir,PG_WR | PG_USER | PG_PRESENT);

               //take note of this, so that it wouldn't cause a memory leak later
               tmpr->vaddr=(DWORD)userheap;
               tmpr->pages=pages;
               tmpr->next=(process_mem*)malloc(sizeof(process_mem));
               tmpr=tmpr->next;
               tmpr->vaddr=0;tmpr->next=0;

               //calculate the position of the reserved heap
               heapres=userheap+(pages*0x1000);

               //allocate "reserved" user heap
               dex32_reserveblock((DWORD)heapres,
                      DEFAULT_HEAPSIZE,&pages,pagedir,PG_WR );


               tmpr->vaddr=(DWORD)heapres;
               tmpr->pages=pages;
               tmpr->next=(process_mem*)malloc(sizeof(process_mem));
               tmpr=tmpr->next;
               tmpr->vaddr=0;tmpr->next=0;

   	
	//examine the sections
	for (i=0;i<totalsections;i++)
		{
			
			int i2;
			char section_name[256];
			DWORD rawdata, vaddr,size,pages,relnum;
			RELOC *relocation;
			
			/* copy the name of the section to a temporary vriable */
			for (i2=0; section[i].s_name[i2]&&i2<8;i2++)
			{
				section_name[i2] = section[i].s_name[i2];
			};
			
			section_name[i2]=0; /*null terminator*/
			
			rawdata = section[i].s_scnptr;           //pointer to raw data in file
			vaddr   = section[i].s_vaddr;            //absolute address in memory
			size    = section[i].s_size;             //size of the section
			relocation = (RELOC*)section[i].s_relptr;  //relocation pointer
			relnum = section[i].s_nreloc;            //number of relocations
			
			#ifdef DEBUG_COFF
				printf("dex32_coff_loader: Section name   [%s]\n",section_name);
				printf("dex32_coff_loader: ** file offset [%d]\n",rawdata);
				printf("dex32_coff_loader: ** load at     [%sH]\n",itoa(vaddr,temp,16));
			#endif
                        
                        /*perform relocations*/
                        for (i2=0;i2<relnum;i2++)
                        {
                        	if (relocation[i2].r_type==RELOC_ADDR32)
                        	{
					DWORD *loc=symbols[relocation[i2].r_symndx].e_value;
                                        printf("dex32_coff_loader: ABSOLUTE RELOCATION Detected\n");
                        	}
                        	else
                        	if (relocation[i2].r_type==RELOC_REL32)
                        	{
                        		printf("dex32_coff_loader: RELATIVE RELOCATION Detected\n");
                        	}
                        	else
                        	printf("dex32_coff_loader: UNKNOWN RELOCATION TYPE\n");
                        	
                	};
                        
                        
                        /*allocate physical memory and assign a virtual address*/
                        #ifdef DEBUG_COFF                        
                        	printf("dex32_coff_loader: Commiting block..\n");
                        #endif
			tmpr->vaddr=(DWORD)dex32_commitblock((DWORD)base+vaddr,size,&pages,
               		pagedir,PG_WR | PG_USER | PG_PRESENT);
               		
               		#ifdef DEBUG_COFF
               			printf("dex32_coff_loader: Copying image at %s to %s..\n",
               			itoa(coff_image+rawdata,temp2,16),itoa(tmpr->vaddr,temp,16));
               		#endif
               		//copy the section to main memory			
  			memcpy(tmpr->vaddr,coff_image+rawdata,size);
 			
 			#ifdef DEBUG_COFF
 				printf("dex32_coff_loader: ***Done***\n");
 			#endif
 			//take note of the memory used so that we could free it up later
                        tmpr->pages=pages;
                        tmpr->next=(process_mem*)malloc(sizeof(process_mem));
                        tmpr=tmpr->next;
                        tmpr->vaddr=0;tmpr->next=0;
                                             
			
		};
		
	     
             dex32_stopints(&flags);
             
             
             ret=createprocess(entrypoint,module_name,pagedir,memptr,stackloc,
             1000,0x2000,signal,p,workdir,parent);
             dex32_restoreints(flags);
             dex32_freeuserpagetable(pagedir1);
             
	return ret; 
  };
return 0; //not a coff file or error loading file
};
