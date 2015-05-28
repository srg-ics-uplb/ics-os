/*
  Name: dex low-level memory management library
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 02/03/04 18:06
  Description: This module handles everything that has to do
  with memory, except the high-level memory functions like malloc....
*/




/* Stop Interrupts */
void stopints()
{
  asm ("cli");
}

/*start Interrupts */

void startints()
{
  asm("sti");
};

void wbinvd()
{
  asm ("wbinvd");
};

void hlt()
{
  asm ("hlt");
};

extern void refreshpages();

/*Displays memory information provided by the GRUB bootloader*/
void mem_interpretmemory(mmap *map,int size)
{
int i;
for (i=0;i<size / sizeof(mmap) ; i++)
  {
   DWORD base = map[i].base_addr_low;
   DWORD base_end = base + map[i].length_low;
   printf("region: 0x%x - 0x%x .",base,base_end);
   if (map[i].type==1) printf("free\n");
      else
      printf("reserved.\n");
  }; 
};

/*using the memory map provided by grub, create the stack of physical frames*/
DWORD mem_detectmemory(mmap *grub_meminfo , int size )
{
DWORD mem_size = 0 , i, i2;
stackbase[0]=0;
for (i=0;i < size / sizeof(mmap) ; i++)
  {
  
   DWORD base = grub_meminfo[i].base_addr_low;
   DWORD base_end = base + grub_meminfo[i].length_low;
   
   /*If type has a value of 1 it is free otherwise we cannot use
     this memory region*/
   if (grub_meminfo[i].type == 1) 
       {
                    for (i2 = base; i2 < base_end; i2 += 0x1000)
                       {
                          if ((stackbase + 0x100000) < i2 )
                             {
                             stackbase[0]++;
                             stackbase[stackbase[0]]=i2;
                             };
                       };

                   mem_size += ( base_end - base );
      };
  }; 
  
totalpages = stackbase[0]; 
return mem_size;   
};



DWORD *mempop()
{
 DWORD *ret;
 if (stackbase[0]==0) return 0; //no more free pages available!
 ret=(DWORD*)stackbase[stackbase[0]];
 stackbase[0]--;
 return ret;
;};

void mempush(DWORD mem)
{
//make sure that the physcial memory location pushed is within
//the range of the computer's physical memory
if (stackbase+0x100000>=mem<memamount-0x1000)
   {
    stackbase[0]++;
    stackbase[stackbase[0]]=mem;
   }
     else
   { 
   char temp[255]; 
   printf("memory manager: An invalid value (%s) was tried to be added to the\n");
   printf("memory manager: free physical pages list.\n");    
   };
};

void clearpagetable(DWORD *pagetable)
   {
     DWORD i;
     for (i=0;i<4096;i++)
        pagetable[i]=0;

   };

/* This function maps the linear address to a physical address:
   The function automatically allocates a page for a page table
   the bottom 12 bits of the linear address is discarded since
   it is not used by the mapper*/
void maplineartophysical(unsigned int *pagedir, /*the location of the page directory*/
                              unsigned int linearaddr, /*the paged aligned linear address requested*/
                              unsigned int physical,   /*the paged aligned physical address to map to*/
									 unsigned int attribute /*used to specify the page attributes to be applied*/
					    )
   {
	  unsigned int pagedirindex,pagetableindex,*pagetable;
	  /*get the index of the page directory and the pagetable respectively*/
	  pagedirindex= linearaddr >> 22;
	  pagetableindex= (linearaddr  & 0x3FFFFF) >> 12;
	  /*get the location of the page table and mask the first 12 bits*/
	  pagetable=(unsigned int*)(pagedir[pagedirindex] & 0xFFFFF000);
	  if (pagetable==0)  /*there is no entry?*/
		     {
				 /*map a new memory location*/
				pagedir[pagedirindex]=(DWORD) mempop();
				pagetable=(unsigned int*)pagedir[pagedirindex];
				/*clear the locations of the page table to zero*/
				memset(pagetable,0,4096);
				/*set the present bit of the pagetable dir entry*/
				pagedir[pagedirindex]=pagedir[pagedirindex] | 1 | PG_USER | PG_WR;

             };
	  physical=(physical & 0xFFFFF000) | attribute;
      pagetable[pagetableindex]=physical;

      /*done!*/


   };

DWORD tlb_address;
extern void invtlb();

/*same as map linear to physical except that it does its tricks without
 modifying the paging bit   */
int maplineartophysical2(unsigned int *pagedir, /*the location of the page directory*/
                              unsigned int linearaddr, /*the paged aligned linear address requested*/
                              unsigned int physical,   /*the paged aligned physical address to map to*/
									 unsigned int attribute /*used to specify the page attributes to be applied*/
					    )
   {
	  unsigned int pagedirindex,pagetableindex,*pagetable;
      DWORD pg;
      DWORD *kicker=(DWORD*)SYS_PAGEDIR2_VIR;
      
	  /*get the index of the page directory and the pagetable respectively*/
	  pagedirindex= linearaddr >> 22;
	  pagetableindex= (linearaddr  & 0x3FFFFF) >> 12;
	  
	  /*get the location of the page table and mask the first 12 bits*/
	  pg=(pagedir[pagedirindex] & 0xFFFFF000);
	  
	  if (pg==0)  /*there is no entry?*/
		     {
				 /*map a new memory location*/
				pagedir[pagedirindex]=(DWORD) mempop();
                kicker[4]=(pagedir[pagedirindex]&0xFFFFF000) | 1;
                
                refreshpages();
                
                pagetable=(DWORD*)SYS_PAGEDIR4_VIR;

                tlb_address = pagetable;
                invtlb();                
				/*clear the locations of the page table to zero*/
				memset(pagetable,0,4096);
				/*set the present bit of the pagetable dir entry*/
				pagedir[pagedirindex] = pagedir[pagedirindex] | 1 | PG_USER | PG_WR;
				refreshpages();
				
				pg = (pagedir[pagedirindex] & 0xFFFFF000);
             };
             
    kicker[4]=pg | 1;
    refreshpages();
        

    pagetable=(DWORD*)SYS_PAGEDIR4_VIR;

    physical = (physical & 0xFFFFF000) | attribute;
    pagetable[pagetableindex]=physical;

    refreshpages();
    
    tlb_address = linearaddr;
    invtlb();
    
    /*done!*/
   };


//quickly gives a physical address a corresponding virtual address
//--used for modifying page tables without disabling paging
DWORD getvirtaddress(DWORD physicaladdr)
{
    DWORD *kicker=(DWORD*)SYS_PAGEDIR2_VIR; //obtain the aux pagetable
    kicker[2]=physicaladdr | 1;
    /*if (current_process->accesslevel==ACCESS_SYS)*/
    refreshpages();
        
    return SYS_PAGEDIR3_VIR;
;};

DWORD getvirtaddress2(DWORD physicaladdr,DWORD hdl)
{
DWORD *kicker=(DWORD*)SYS_PAGEDIR2_VIR; //obtain the aux pagetable
kicker[hdl]=physicaladdr | 1;
/*if (current_process->accesslevel==ACCESS_SYS)*/
refreshpages();

return SYS_PAGEDIR_VIR+hdl;
;};




DWORD xmaplineartophysical(const DWORD linearmemory,const DWORD physicalmemory,
   DWORD *pagedir,const DWORD attb) //ATOMIC
    {
     DWORD w=linearmemory;
     DWORD dirindex=(w&0xFFC00000) >> 22;
     DWORD pageindex=(w&0x3FF000) >> 12;
     DWORD *pagetbl;
     DWORD flags;
     dex32_stopints(&flags);
     disablepaging();
     if (pagedir[dirindex]&PG_PRESENT==0) //no page table allocated?
        {
         pagetbl=mempop();
         if (pagetbl==0) {dex32_restoreints(flags);enablepaging();return 0;};
         pagedir[dirindex]=(DWORD)pagetbl | 1;
        };
        
     if (pagedir[dirindex]&1)
      {
       pagetbl=(DWORD*)(pagedir[dirindex]&0xFFFFF000);
       pagetbl[pageindex]=physicalmemory | (attb&0xFFF);
       
       dex32_restoreints(flags);
       enablepaging();
       return 1;
      }
      
     dex32_restoreints(flags);
     enablepaging();
    return 0;
    };

int ints_enabled=1;

void dex32_stopints(DWORD *flags)
 {
   storeflags(flags);
   stopints();
 ;};

void dex32_restoreints(DWORD flags)
 {
   restoreflags(flags);
 ;};



DWORD getphys(DWORD vaddr,DWORD *pagedir)
   {
     DWORD dirindex=(vaddr&0xFFC00000) >> 22;
     DWORD pageindex=(vaddr&0x3FF000) >> 12;
     DWORD *pagetbl;
     DWORD ret;
     DWORD *pg;

     pg=(DWORD*)getvirtaddress((DWORD)pagedir);
     if (pg[dirindex]&1==0) return 0;
     pagetbl=(DWORD*)(pg[dirindex]&0xFFFFF000);
     pg=(DWORD*)getvirtaddress((DWORD)pagetbl);
     ret=pg[pageindex];
     return ret;
   ;};

DWORD getpagetablephys(DWORD vaddr,DWORD *pagedir)
   {
     DWORD dirindex=(vaddr&0xFFC00000) >> 22;
     DWORD *pg;
     pg=(DWORD*)getvirtaddress((DWORD)pagedir);
     return pg[dirindex];
   ;};

void dex32_freeuserpagetable(DWORD *pgd)  //ATOMIC function
   {
     DWORD userstart=(DWORD)userspace >> 22,
           userend  =0xC0000000 >> 22;
     DWORD auxstart=0xFFC00000 >> 22;
     DWORD *pagedir,cpuflags;
     DWORD *pagetbl,address;
     DWORD i;
     DWORD pages=0;
     storeflags(&cpuflags);
     stopints();
     
     pagedir =(DWORD*)getvirtaddress((DWORD)pgd);
     for (i=userstart;i<userend;i++)
          if (pagedir[i]&1) //check if present
             {
               mempush(pagedir[i]&0xFFFFF000);
               pages++;
               pagedir[i]=0;
             };
             
     if (pagedir[auxstart]&1&&(pgd!=pagedir1))
             {
               mempush(pagedir[auxstart]&0xFFFFF000);
               pages++;
               pagedir[auxstart]=0;
             };
     restoreflags(cpuflags);
     #ifdef MEM_LEAK_CHECK
     printf("freeuserpagetable() frees %d pages.\n",pages);
     #endif
   };


void freeuserheap(DWORD *pagedir)
   {
   DWORD start=0xA0000000,end=0xB0000000;
   DWORD i;
   for (i=start;i<end;i++)
      {
      DWORD w=getphys(i,pagedir);
      if (w&1)
      {
        mempush(w&0xFFFFF000);
        maplineartophysical2(pagedir,i,0,0);
       };
      };
  };


void freelinearloc(void *linearmemory,DWORD *pagedir)  //ATOMIC function
   {
     DWORD w=(DWORD)linearmemory;
     DWORD dirindex=(w&0xFFC00000) >> 22;
     DWORD pageindex=(w&0x3FFFFF) >> 12;
     DWORD *pagetbl,address,flags;
     char temp[255];

     dex32_stopints(&flags);
    // disablepaging();
     address = getphys(linearmemory,pagedir);
    /* pagetbl=(DWORD*)(pagedir[dirindex]&0xFFFFF000);
     address=pagetbl[pageindex];*/
     if (address&1)
     mempush(address&0xFFFFF000);
    //pagetbl[pageindex]=0;
    // enablepaging();
     dex32_restoreints(flags);

   };

//frees multiple pages and returns them back to the stack
void freemultiple(void *linearmemory,DWORD *pagedir,DWORD pages)
  {
  int i;
  #ifdef MEM_LEAK_CHECK
  printf("freemultiple() frees %d pages.\n",pages);
  #endif
  for (i=0;i<pages;i++)
    {
      freelinearloc(linearmemory,pagedir);
      linearmemory=(void*)(linearmemory+0x1000);
    };
  };

DWORD getlinearloc(void *linearmemory,DWORD *pagedir)  //ATOMIC function
   { 
     DWORD w=(DWORD)linearmemory,ret=0,*pg;
     DWORD dirindex=(w&0xFFC00000) >> 22;
     DWORD pageindex=(w&0x3FFFFF) >> 12;
     DWORD *pagetbl,address,flags;
     char temp[255];

     dex32_stopints(&flags);
     pg=(DWORD*)getvirtaddress((DWORD)pagedir); //convert to a virtual address so that
               
     pagetbl=(DWORD*)(pg[dirindex]&0xFFFFF000);
     if (pagetbl!=0)
     {
          pg=(DWORD*)getvirtaddress((DWORD)pagetbl); //convert to a virtual address so that
          address=pg[pageindex];
          if (address&1)
          ret = 1;
     }
     else
     ret =0;    
     
     dex32_restoreints(flags);
     return ret;
   };


//Determines the amount of physical pages commited based on a
//given linear memory
DWORD getmultiple(void *linearmemory,DWORD *pagedir,DWORD pages)
  {
  int i;
  DWORD total=0;
  
  for (i=0;i<pages;i++)
    {
      total+=getlinearloc(linearmemory,pagedir);
      linearmemory=(void*)(linearmemory+0x1000);
    };
    return total;
  };


void setpageattb(DWORD *pagedir,DWORD vaddr,DWORD attb)
  {
    DWORD pg,phys;
    phys=getphys(vaddr,pagedir);
    pg=(DWORD)getvirtaddress((DWORD)pagedir);

    maplineartophysical2(pg,vaddr,
    phys,attb);
          //   printf("ok\n");

  };


void *dex32_setpageattb(DWORD virtualaddr,DWORD pages,DWORD *pagedir,DWORD pattb)
    {
     int i;
     char temp[255],temp2[255];
     void *ret=(void*)virtualaddr;
     DWORD *pg;
     for (i=0;i<pages;i++)
        {
          DWORD pageadr;
          setpageattb(pagedir,virtualaddr,pattb);
          if (current_process->accesslevel==ACCESS_SYS)
          setpageattb(pagedir1,virtualaddr,pattb);
          virtualaddr+=0x1000;
         };
     refreshpages();
     return ret;
    };

void *dex32_setpageattbblock(DWORD virtualaddr,int amt,DWORD *pagecount,DWORD *pagedir,DWORD attb)
   {
     int pages=(amt/4096)+1;
     char *ret=0;
     if (amt==0) pages=1;
        else
     if (amt%4096==0) pages=amt/4096;
     ret=dex32_setpageattb(virtualaddr,pages,pagedir,attb);
     *pagecount=pages;
     return ret;
   };

//returns the total number of free physical pages on the system

DWORD dex32_getfreepages()
   {
       return totalpages; //the totalpages global variable holds the number of pages left

   };



void setgdtentry(DWORD index,void *base,gdtentry *t,DWORD limit,
           BYTE attb1,BYTE attb2)
   {
      DWORD b=(DWORD)base;
      t->lowaddr=b;
      t->lowaddr2=b >> 16;
      t->limit=limit;
      t->att1=attb1;
      t->att2=attb2;
      t->highaddr=b >> 24;
   };

#define RING0_TSS 0x89

WORD addgdt(DWORD base,DWORD limit,BYTE attb1,BYTE attb2)
  {
    int index=totalgdtentries;
    dex_gdtbase[index].lowaddr=base;
    dex_gdtbase[index].lowaddr2=base >> 16;
    dex_gdtbase[index].limit=limit;
    dex_gdtbase[index].att1=attb1;
    dex_gdtbase[index].att2=attb2 | ((limit >> 8)&0xF);
    dex_gdtbase[index].highaddr=base >> 24;
    return (index << 3);
  };


void setgdt(WORD sel,DWORD base,DWORD limit,BYTE attb1,BYTE attb2)
  {
    sel=sel >> 3;
    dex_gdtbase[sel].lowaddr=base;
    dex_gdtbase[sel].lowaddr2=base >> 16;
    dex_gdtbase[sel].limit=limit;
    dex_gdtbase[sel].att1=attb1;
    dex_gdtbase[sel].att2=attb2 | ((limit >> 16)&0xF);
    dex_gdtbase[sel].highaddr=base >> 24;
  };

void setattb(WORD sel,BYTE attb1)
  {
    sel=sel >> 3;
    dex_gdtbase[sel].att1=attb1;
  };

void setcallgate(DWORD sel,DWORD funcsel,void *entry,BYTE params,BYTE access)
  {
    CALLGATE *ptr=(CALLGATE*)dex_gdtbase;
    DWORD loc=(DWORD)entry;
    sel=sel>>3 ; //convert selector to an index
    ptr[sel].lowoffset=loc;
    ptr[sel].selector=funcsel;
    ptr[sel].attb1=params;
    ptr[sel].attb2=0x8c | (access << 5 );
    ptr[sel].highoffset=loc >> 16;
  };
extern void reset_gdtr();

void dex32_setbase(WORD sel,DWORD addr)
  {
    DWORD cpuflags;
//    storeflags(&cpuflags);
//    stopints();
    sel=sel >> 3;
    dex_gdtbase[sel].lowaddr=addr;
    dex_gdtbase[sel].lowaddr2=addr >> 16;
    dex_gdtbase[sel].highaddr=addr >> 24;
 //   reset_gdtr();
//    restoreflags(cpuflags);
  };



void  setinterruptvector(DWORD index,idtentry *t,unsigned char attr,
     void (*handler)(int irq), WORD sel)
	{

    t[index].lowphy=(WORD)handler; //set the low word
    t[index].highphy=((DWORD)handler >> 16);	//set the high word
    t[index].selector=sel;
	 t[index].reserved=0;
	 t[index].attr=attr;
    };

DWORD obtainpage()
    {
      print("DEX 32: out of memory error\n");
      return 0;
    };



void *commit(DWORD virtualaddr,DWORD pages)
    {
     int i;
     char temp[255];
     void *ret=(void*)virtualaddr;
     DWORD flags;
     storeflags(&flags);
     stopints();
     
     #ifdef MEM_LEAK_CHECK
     printf("system committed %d pages.\n",pages);
     #endif
   
     for (i=0;i<pages;i++)
        {
          DWORD pageadr=(DWORD)mempop();
          
          //if out of physical address, call the VMM
          if (pageadr==0) pageadr=obtainpage();
          
          //out of memory error
          if (pageadr == -1) {ret = -1;break;};
          
          maplineartophysical2((DWORD*)SYS_PAGEDIR_VIR,virtualaddr,pageadr,PG_PRESENT);
          maplineartophysical2((DWORD*)SYS_KERPDIR_VIR,virtualaddr,pageadr,PG_PRESENT);
          virtualaddr+=0x1000;

        };
        
     restoreflags(flags);   
   
     return ret;
    };

void *commitb(DWORD virtualaddr,int amt,DWORD *pagecount)
   {
     int pages=(amt/4096)+1;
     char *ret=0;
     if (amt==0) pages=1;
        else
     if (amt%4096==0) pages=amt/4096;
     ret=commit(virtualaddr,pages);
     /*DEBUG*/

     *pagecount=pages;
     return ret;
   };

void *sbrk(int amt)
   {
     int pages=(amt/4096)+1;
     char *ret=0;
     
     //cannot handle negative values as of the moment
     if (amt<0) return -1;
     
     //return location of break
     if (amt==0) return knext-1;
     
     if (amt%4096==0) pages=amt/4096;
     
     ret=commit((DWORD)knext,pages);

     knext+=(pages)*4096;
     
     return (void*)ret;
   };



//defines the USER MODE function for changing the break of an
//application
void *dex32_sbrk(unsigned int amt)
   {
     DWORD pages=(amt/4096)+1;
     DWORD flags;
     char *ret=current_process->knext,temp[255];
     dex32_stopints(&flags);
     if (amt==0)
        {
        dex32_restoreints(flags);
        
        return ((void*)current_process->knext);
        };
     if (amt%4096==0) pages=amt/4096;

   /*  ret=dex32_commit((DWORD)current_process->knext,
                 pages,(DWORD*)SYS_PAGEDIR_VIR,
                  PG_USER | PG_WR);*/
     //increases the heap of the process
     //  addmemusage(&(current_process->meminfo),ret,pages);

     current_process->knext+=pages*4096;
     dex32_restoreints(flags);
     return (void*)ret;
   };


void dex32_copy_on_write(DWORD *directory)
{
  /*   DWORD i;
     DWORD userstart=(DWORD)userspace >> 22,
           userend  =0xC0000000 >> 22,


     for (i=userstart;i<=userend;i++)
       {


       ;};

    */
;};


//copies a pagedirectory except the user space area
//0xA0000000-0xC0000000
void dex32_copy_pagedirU(DWORD *destdir,DWORD *source)
   {
     DWORD i;
     DWORD userstart=(DWORD)userspace >> 22,
           userend  =0xC0000000 >> 22,
           stopaddr =0xF0000000 >> 22;
     disablepaging();
    // memset(destdir,0,0x1000);
     for (i=0;i<userstart;i++)
        destdir[i]=source[i];
     for (i=userend;i<stopaddr;i++)
        destdir[i]=source[i];
   };

int dex32_copy_pg(DWORD *destdir, DWORD *source)
   {
      DWORD i,i2;
      DWORD start = (DWORD) userspace >> 22;
      DWORD end = 0xC0000000 >> 22;
      DWORD syscallstart = 0x90000000 >> 22, syscallend=0xA0000000 >> 22;
      dex32_copy_pagedirU(destdir,source);
      for (i=start;i<end;i++)
        {
          if (source[i]&1)
            {
                  DWORD *destpagetable,*srcpagetable;
                  destdir[i]= (DWORD)mempop() | 1 | PG_USER | PG_WR;

                  destpagetable= (DWORD)destdir[i]&0xFFFFF000;
                  srcpagetable = (DWORD)source[i]&0xFFFFF000;

                  for (i2= 0 ;i2 < 1024; i2++)
                  {

                     if (srcpagetable[i2]&1)
                      {
                 		      DWORD free_mem = (DWORD)mempop();
                              //out of memory?
                              if (free_mem==-1) return -1;

                              destpagetable[i2] = free_mem | 1;

                              if (i>=syscallstart&&i<syscallend)
                                  {
                                        memcpy(destpagetable[i2]&0xFFFFF000,srcpagetable[i2]&0xFFFFF000,0x1000);
                                  }
                                    else
                                  {
                                  		memcpy(destpagetable[i2]&0xFFFFF000,srcpagetable[i2]&0xFFFFF000,0x1000);
                                        destpagetable[i2]= destpagetable[i2] | PG_WR | PG_USER;
                                  };
                      }
                        else
                     destpagetable[i2]= srcpagetable[i2];
                  };
            }
              else
            destdir[i] = source[i];
        };
   };

//copies a pagedirectory given a source and a destination
void dex32_copy_pagedir(DWORD *destdir,DWORD *source)
   {
     int i;
     for (i=0;i<(0x1000/2);i++)
        destdir[i]=source[i];
   };

//designed for  USER MODE applications
//commits a block of memory at a time in bytes
void *dex32_commitblock(DWORD virtualaddr,int amt,
    DWORD *pagecount,DWORD *pagedir,DWORD attb)
   {
     int pages=(amt/4096)+1;
     char *ret=0;
     if (amt==0) pages=1;
        else
     if (amt%4096==0) pages=amt/4096;      
     
     ret=dex32_commit(virtualaddr,pages,pagedir,attb);
     
     *pagecount=pages;
     return ret;
   };


void *dex32_reserveblock(DWORD virtualaddr,int amt,
    DWORD *pagecount,DWORD *pagedir,DWORD attb)
   {
     int pages=(amt/4096)+1;
     char *ret=0;
     if (amt==0) pages=1;
        else
     if (amt%4096==0) pages=amt/4096;
     ret=dex32_reserve(virtualaddr,pages,pagedir,attb);
     *pagecount=pages;
     return ret;
   };
//sets a virtual address location as demand paged..
//meaning, the OS commits a pge only if it has been accessed
void *dex32_reserve(DWORD virtualaddr,DWORD pages,DWORD *pagedir,DWORD attb)
   {
     int i;
     char temp[255],temp2[255];
     void *ret=(void*)virtualaddr;
     DWORD *pg;
     for (i=0;i<pages;i++)
        {
          DWORD pageadr; //holds a physical frame, if possible

          pg=(DWORD*)getvirtaddress((DWORD)pagedir);
          maplineartophysical2(pg,virtualaddr,
          0,PG_USER | PG_DEMANDLOAD | attb);

          if (current_process->accesslevel==ACCESS_SYS)
          {
          pg=(DWORD*)getvirtaddress((DWORD)pagedir1);
          maplineartophysical2(pg,virtualaddr,
          pageadr,PG_DEMANDLOAD | PG_USER | attb);
          virtualaddr+=0x1000;
          };
         };
    refreshpages();
     return ret;
    };

//designed for USER MODE applications
//commits a block of memory at a time in pages
void *dex32_commit(DWORD virtualaddr,DWORD pages,DWORD *pagedir,DWORD pattb)
    {
     int i;
     char temp[255],temp2[255];
     void *ret=(void*)virtualaddr;
     DWORD *pg;
     DWORD flags;
     
     storeflags(&flags);
     stopints();

     /*DEBUG*/

     #ifdef MEM_LEAK_CHECK
     printf("user committed %d pages.\n",pages);
     #endif
     for (i=0;i<pages;i++)
        {
          DWORD pageadr; //holds a physical frame, if possible
        //if page is set to demand paged, we do not commit
        //a physical frame yet
          pageadr=(DWORD)mempop();

          if (pageadr==0) //we're out of physical frames, find a way to get one
          pageadr=obtainpage(); //call the VMM to get a page


          pg=(DWORD*)getvirtaddress((DWORD)pagedir);


          maplineartophysical2(pg,virtualaddr, pageadr,PG_PRESENT | /*PG_USER |*/ pattb);
              //if this is called from the KERNEL map it to kernel memory too
              if (current_process->accesslevel==ACCESS_SYS)
               {
                   pg=(DWORD*)getvirtaddress((DWORD)pagedir1);
        
                   maplineartophysical2(pg,virtualaddr,
                   pageadr,PG_PRESENT | /*PG_USER |*/ pattb);
               };
           
          virtualaddr+=0x1000;
         };
         
     refreshpages();
     restoreflags(flags);
     
     return ret;
    };

void dex32copyblock(DWORD vdest,DWORD vsource,DWORD pages,DWORD *pagedir)
   {
     int i;
     for (i=0;i<pages;i++)
          {
           DWORD pdest=getphys(vdest,pagedir);
           DWORD pg=(DWORD*)getvirtaddress((DWORD)pdest);
           memcpy((void*)pg,(void*)vsource,0x1000);
           vdest+=0x1000;
           vsource+=0x1000;
          };

   };


void mem_init()
{
    DWORD i;
    char temp[255];
    pagedir1=mempop(); //obtain the first pagedirectory

    clearpagetable(pagedir1);
    //map the first 1MB
    for (i=0;i<0xB8000;i+=0x1000)
        maplineartophysical((DWORD*)pagedir1,i,i        /*,stackbase*/,1 );
    for (i=0xb8000;i<0x100000;i+=0x1000)
        maplineartophysical((DWORD*)pagedir1,i,i        /*,stackbase*/,1 | PG_USER | PG_WR );
    for (i=0x100000;i<0x300000;i+=0x1000)
        maplineartophysical((DWORD*)pagedir1,i,i        /*,stackbase*/,1);

    maplineartophysical((DWORD*)pagedir1,(DWORD)SYS_PAGEDIR_VIR,(DWORD)pagedir1    /*,stackbase*/,1);
    maplineartophysical((DWORD*)pagedir1,(DWORD)SYS_PAGEDIR2_VIR,
    (DWORD)pagedir1[SYS_PAGEDIR_VIR >> 22]&0xFFFFF000,1);
    maplineartophysical((DWORD*)pagedir1,(DWORD)SYS_PAGEDIR3_VIR,
    (DWORD)pagedir1[SYS_PAGEDIR_VIR >> 22]&0xFFFFF000,1);
    maplineartophysical((DWORD*)pagedir1,(DWORD)SYS_KERPDIR_VIR,(DWORD)pagedir1    /*,stackbase*/,1);

    setpagedir(pagedir1);
    enablepaging();
};

/*This function registers the memory amanger to the device manager so that it's
  interface could be used by other device drivers*/
void mem_register()
{
devmgr_mem memory_manager;

//Set up the usual things the device manager needs
memory_manager.hdr.size = sizeof(devmgr_mem);
strcpy(memory_manager.hdr.name,"mem_mgr");
strcpy(memory_manager.hdr.description,"DEX low-level memory manager");
memory_manager.hdr.type = DEVMGR_MEM;

/* Fill up the service functions that the device manager provices, this
   functions will be visible to other modules that use the mem_mgr interface*/
   
memory_manager.sbrk = sbrk;
memory_manager.mem_map = maplineartophysical2;
memory_manager.commit = commit;
memory_manager.freemultiple = freemultiple;

/*register myself to the device manager*/
devmgr_register( (devmgr_generic*) &memory_manager );

};

