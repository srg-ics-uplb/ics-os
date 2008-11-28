/*
  Name: 32-bit PE format executable loader
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 13/03/04 06:31
  Description: this module specifically loads 32-bit WIN32 PE format executables and libraries.
  This implementation is based on the document "Microsoft Portable Executable and
  Common Object File Format Specification Revision 6.0 " released by Microsoft Corporation 
  on February 1999.
  
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

#define ATTB_CODE     0x00000020
#define ATTB_INITDATA 0x00000040
#define ATTB_BSS      0x00000080
#define ATTB_SHARED   0x10000000
#define ATTB_EXECUTE  0x20000000
#define ATTB_READ     0x40000000
#define ATTB_WRITE    0x80000000

#define EXPORT_TABLE 0

#define PE_USEREXE 0
#define PE_USERDLL 1
#define PE_SYSEXE  2
#define PE_SYSDLL  3


typedef struct __attribute__((packed)) _IMAGE_EXPORT_DIRECTORY {
    DWORD   Characteristics;
    DWORD   TimeDateStamp;
    WORD    MajorVersion;
    WORD    MinorVersion;
    DWORD   Name;
    DWORD   Base;
    DWORD   NumberOfFunctions;
    DWORD   NumberOfNames;
    DWORD  *AddressOfFunctions;
    DWORD  *AddressOfNames;
    WORD   *AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;



//these structs are replicated from the structures found in winnt.h
typedef struct __attribute__((packed)) _IMAGE_DOS_HEADER {      // DOS .EXE header
    WORD   e_magic;                     // Magic number
    WORD   e_cblp;                      // Bytes on last page of file
    WORD   e_cp;                        // Pages in file
    WORD   e_crlc;                      // Relocations
    WORD   e_cparhdr;                   // Size of header in paragraphs
    WORD   e_minalloc;                  // Minimum extra paragraphs needed
    WORD   e_maxalloc;                  // Maximum extra paragraphs needed
    WORD   e_ss;                        // Initial (relative) SS value
    WORD   e_sp;                        // Initial SP value
    WORD   e_csum;                      // Checksum
    WORD   e_ip;                        // Initial IP value
    WORD   e_cs;                        // Initial (relative) CS value
    WORD   e_lfarlc;                    // File address of relocation table
    WORD   e_ovno;                      // Overlay number
    WORD   e_res[4];                    // Reserved words
    WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
    WORD   e_oeminfo;                   // OEM information; e_oemid specific
    WORD   e_res2[10];                  // Reserved words
    DWORD   e_lfanew;                    // File address of new exe header
  } IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

 typedef struct __attribute__((packed)) _IMAGE_FILE_HEADER {
    WORD    Machine;
    WORD    NumberOfSections;
    DWORD   TimeDateStamp;
    DWORD   PointerToSymbolTable;
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader;
    WORD    Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct __attribute__((packed)) _IMAGE_DATA_DIRECTORY {
    DWORD   VirtualAddress;
    DWORD   Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16

//
// Optional header format.
//

typedef struct __attribute__((packed)) _IMAGE_OPTIONAL_HEADER {
    //
    // Standard fields.
    //

    WORD    Magic;
    BYTE    MajorLinkerVersion;
    BYTE    MinorLinkerVersion;
    DWORD   SizeOfCode;
    DWORD   SizeOfInitializedData;
    DWORD   SizeOfUninitializedData;
    DWORD   AddressOfEntryPoint;
    DWORD   BaseOfCode;
    DWORD   BaseOfData;

    //
    // NT additional fields.
    //

    DWORD   ImageBase;
    DWORD   SectionAlignment;
    DWORD   FileAlignment;
    WORD    MajorOperatingSystemVersion;
    WORD    MinorOperatingSystemVersion;
    WORD    MajorImageVersion;
    WORD    MinorImageVersion;
    WORD    MajorSubsystemVersion;
    WORD    MinorSubsystemVersion;
    DWORD   Win32VersionValue;
    DWORD   SizeOfImage;
    DWORD   SizeOfHeaders;
    DWORD   CheckSum;
    WORD    Subsystem;
    WORD    DllCharacteristics;
    DWORD   SizeOfStackReserve;
    DWORD   SizeOfStackCommit;
    DWORD   SizeOfHeapReserve;
    DWORD   SizeOfHeapCommit;
    DWORD   LoaderFlags;
    DWORD   NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER, *PIMAGE_OPTIONAL_HEADER;

typedef struct __attribute__((packed)) _IMAGE_ROM_OPTIONAL_HEADER {
    WORD   Magic;
    BYTE   MajorLinkerVersion;
    BYTE   MinorLinkerVersion;
    DWORD  SizeOfCode;
    DWORD  SizeOfInitializedData;
    DWORD  SizeOfUninitializedData;
    DWORD  AddressOfEntryPoint;
    DWORD  BaseOfCode;
    DWORD  BaseOfData;
    DWORD  BaseOfBss;
    DWORD  GprMask;
    DWORD  CprMask[4];
    DWORD  GpValue;
} IMAGE_ROM_OPTIONAL_HEADER, *PIMAGE_ROM_OPTIONAL_HEADER;
#define IMAGE_SIZEOF_SHORT_NAME              8

typedef struct _IMAGE_SECTION_HEADER {
    BYTE    Name[IMAGE_SIZEOF_SHORT_NAME];
    union {
            DWORD   PhysicalAddress;
            DWORD   VirtualSize;
    } Misc;
    DWORD   VirtualAddress;
    DWORD   SizeOfRawData;
    DWORD   PointerToRawData;
    DWORD   PointerToRelocations;
    DWORD   PointerToLinenumbers;
    WORD    NumberOfRelocations;
    WORD    NumberOfLinenumbers;
    DWORD   Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

typedef struct _IMAGE_BASE_RELOCATION {
    DWORD   VirtualAddress;
    DWORD   SizeOfBlock;
    WORD    TypeOffset[1];
} IMAGE_BASE_RELOCATION;

typedef struct _IMAGE_IMPORT_BY_NAME {
    WORD    Hint;
    BYTE    Name[1];
} IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;

typedef struct _IMAGE_THUNK_DATA {
    union {
        BYTE  *ForwarderString;
        DWORD *Function;
        DWORD Ordinal;
        PIMAGE_IMPORT_BY_NAME AddressOfData;
    } u1;
} IMAGE_THUNK_DATA;
typedef IMAGE_THUNK_DATA * PIMAGE_THUNK_DATA;


typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    union {
        DWORD   Characteristics;                // 0 for terminating null import descriptor
        PIMAGE_THUNK_DATA OriginalFirstThunk;   // RVA to original unbound IAT
         }u;

    DWORD   TimeDateStamp;                  // 0 if not bound,
                                            // -1 if bound, and real date\time stamp
                                            //     in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT (new BIND)
                                            // O.W. date/time stamp of DLL bound to (Old BIND)

    DWORD   ForwarderChain;                 // -1 if no forwarders
    DWORD   Name;
    PIMAGE_THUNK_DATA FirstThunk;           // RVA to IAT (if bound this IAT has actual addresses)
} IMAGE_IMPORT_DESCRIPTOR;



#define IMAGE_SIZEOF_BASE_RELOCATION         8

//
// Based relocation types.
//

#define IMAGE_REL_BASED_ABSOLUTE              0
#define IMAGE_REL_BASED_HIGH                  1
#define IMAGE_REL_BASED_LOW                   2
#define IMAGE_REL_BASED_HIGHLOW               3
#define IMAGE_REL_BASED_HIGHADJ               4
#define IMAGE_REL_BASED_MIPS_JMPADDR          5
#define IMAGE_REL_BASED_SECTION               6
#define IMAGE_REL_BASED_REL32                 7

#define IMAGE_NT_SIGNATURE 0x00004550

#define KERNEL_MODE 0
#define USER_MODE 1
#define HACK_MODE 0xFF

//prototypes of the functions
DWORD patchimportedfunction(const char *fxn_name,char *module);
void importpatch(IMAGE_IMPORT_DESCRIPTOR *dir,char *base,DWORD *pagedir);

DWORD win32_unimplemented()
 {
  printf("Function not implemented\n");
  return 0;
 };

//converts wierd borland style function exports to a simpler kind
void conexpname(const char *strx,char *str)
  {
  int i,i2=0,trail=1;
  for (i=0;strx[i];i++)
   {
     if (trail&&strx[i]=='_') continue;
     if (strx[i]=='@') continue;
     if (strx[i]=='$') break;
     str[i2]=strx[i];trail=0;
     i2++;
   };
  str[i2]=0;
 };


//this function is used to find imported functions and patch it
//if capable, it will also be usd to automatically load libaries
//from the disk if needed
void importpatch(IMAGE_IMPORT_DESCRIPTOR *dir,char *base,DWORD *pagedir)
 {
     while (dir->u.Characteristics)
       {
          IMAGE_THUNK_DATA *data,*ndata;IMAGE_IMPORT_BY_NAME *imp;
          char *modulename=(char*)(base+dir->Name);
          int i=0;
          
          #ifdef MODULE_DEBUG
          printf("Module or DLL:%s\n",modulename);
          #endif
          
          data=(IMAGE_THUNK_DATA*)((DWORD)base+(DWORD)dir->u.OriginalFirstThunk);
          ndata=(IMAGE_THUNK_DATA*)((DWORD)base+(DWORD)dir->FirstThunk);
          if (dir->TimeDateStamp==-1)  //new bind?
          {
          	printf("new binding not yet supported!\n");
            return;
          };
        //load the library if it is not available in memory
          if (!patchimportedfunction(0,modulename))
            {
               //loadSystemDLL(modulename,0);
            };

          while (data->u1.Ordinal)
           {
              DWORD *w=(DWORD*)ndata;
              imp=(IMAGE_IMPORT_BY_NAME*)((DWORD)base+(DWORD)data->u1.AddressOfData);
              *w=patchimportedfunction(imp->Name,modulename);

              if (*w==0) //place a placeholder in case patching has failed
              {
              
              printf("Warning: %d. Cannot resolve '%s'\n",i,imp->Name);
              
              *w=(DWORD)0xFFFF0000+i;
              };
              data++;ndata++;
              i++;
           };
         dir++;
       };
;};


int module_call_library(void *entrypoint, lib_PCB *lib_info, char *parameters)
{
  int (*libentrypoint)(devmgr_generic *ptr,char *p);
  char temp[255];
  devmgr_generic *interface=devmgr_getdevice(devmgr_finddevice("devmgr"));
  libentrypoint = entrypoint;
  
  printf("Calling dex32 compatible library entrypoint located at %s..\n",
           itoa(libentrypoint,temp,16));
  
  /*Check if module returned an error.. if so we abort the load process*/
  if (libentrypoint(interface,parameters) == -1) 
      {
      //free the memory used by the library
      freeprocessmemory(lib_info->memptr,pagedir1);
      return -1; //return with error
      };
      
  printf("Call successful.\n");
  return 0;
};

//Unloads a library from memory
int module_unload_library(const char *name)
{
 lib_PCB *libhead  = firstlibrary;
 lib_PCB *ptr=libhead,*prev=0;
 if (ptr==0) return -1;
 do {

    if (strcmp(name,ptr->name)==0)
    {
         //call the library's exit function
         if (ptr->libexit!=0)
                ptr->libexit();
                
         if (prev!=0)
             prev->next=ptr->next;
         else
         	 firstlibrary=ptr->next;

         freeprocessmemory(ptr->memptr,pagedir1);
         return 1;
    };
   prev=ptr;
   ptr=ptr->next;
 } while (ptr!=0);
 return -1;
};

//gets the virtual address of an exported function from a library with <moduleid>
DWORD module_getfxn(int moduleid,const char *func_name)
{
lib_PCB *ptr=firstlibrary;
//lopp through the installed libraries in memory
while (ptr!=0)
      {
          if (ptr->id == moduleid)
                {
                  DWORD *fxn_names,*fxn_addr,i;              
                  IMAGE_EXPORT_DIRECTORY *export_directory;
                  //obtain the pointer to the export directory
                  export_directory = ptr->exporttable;
                  if (export_directory->AddressOfNames == 0 ) return -1;
                  fxn_names=(DWORD*)((DWORD)export_directory->AddressOfNames+ptr->baseaddress);
                  fxn_addr=(DWORD*)((DWORD)export_directory->AddressOfFunctions+ptr->baseaddress);
                  for (i=0;i< export_directory->NumberOfNames;i++)
                      {
                        char fxnname[256];
                        strcpy( fxnname,(char*)((DWORD)fxn_names[i]+ptr->baseaddress));
                        //check if this is the function we are looking for
                        if (strcmp(fxnname,func_name)==0) 
                             return(fxn_addr[i]+ptr->baseaddress);  
                       };     
                  };  
           ptr=ptr->next;
      };
   return -1;   
};

//lists all exported functions in a particular library
void module_listfxn(const char *modulename)
{
lib_PCB *ptr=firstlibrary;
//search the installed libraries
while (ptr!=0)
  {
   if (strcmp(ptr->name,modulename)==0) //we have found the correct module
         {
           DWORD *fxn_names,*fxn_addr,i;
           IMAGE_EXPORT_DIRECTORY *export_directory;
           char temp[255];
           //obtain the pointer to the export directory
           export_directory = ptr->exporttable;
           printf("Base of library at %s\n",itoa(ptr->baseaddress,temp,16));
           if (export_directory->AddressOfNames == 0 ) return 1;
           printf("Function listing of %s\n",modulename);      
           fxn_names=(DWORD*)((DWORD)export_directory->AddressOfNames+ptr->baseaddress);
           fxn_addr=(DWORD*)((DWORD)export_directory->AddressOfFunctions+ptr->baseaddress);
           printf("-----------------------\n");
           for (i=0;i< export_directory->NumberOfNames;i++)
           {
             char fxnname[256],s2[255];
             DWORD fxnaddress = fxn_addr[i]+ptr->baseaddress;
             strcpy( fxnname, (char*)((DWORD)fxn_names[i]+ptr->baseaddress));
             printf("%d. %-30s : %s\n",i,fxnname,itoa(fxnaddress,temp,16));
           };  
           printf("done.\n");          
         };
     ptr=ptr->next;
  };
 return 0; //failed to find the module
};

//this function is used if the image cannot be placed in the prefered
//relocation area, so the addresses in the image must be modified
//accordingly

void relocate(IMAGE_BASE_RELOCATION *relocbase,DWORD actualload,
     DWORD preferedload,DWORD size)
  {
    DWORD adjust;
    char t1[20],t2[20];
    int i;
    int delta;
    IMAGE_BASE_RELOCATION *table = relocbase;
    //compute the difference that will be added to the
    //relocations
    if (actualload==preferedload) return; //no relocs necessary

    #ifdef DEBUG_PEMODULE
    printf("relocating module..\n");
    printf("preferedload: %s, actualload: %s\n",itoa(preferedload,t1,16),
            itoa(actualload,t2,16));
	 pause();
    #endif

    delta = actualload- preferedload;

    while (table->VirtualAddress!=0)
    {
    char str[20];
    int numberofitems=(table->SizeOfBlock-8)/2;
    DWORD baseptr=(DWORD)(table->VirtualAddress+actualload);

    #ifdef DEBUG_PEMODULE
    printf("size of relocation block: %d\n", table->SizeOfBlock);
    printf("relocbase virtual address: %s\n", itoa(table->VirtualAddress, t1, 16));
    pause();
    #endif

    for (i=0;i<numberofitems;i++)
       {
       DWORD offset = table->TypeOffset[i]&0xFFF;
       int type = table->TypeOffset[i] >> 12;
       DWORD *dd;
       WORD  *dw;
       dw=(WORD*)(baseptr+offset);
       dd=(DWORD*)(baseptr+offset);

       if (type ==IMAGE_REL_BASED_ABSOLUTE)
       	     {
              #ifdef DEBUG_PEMODULE
              printf("IMAGE_REL_BASED_ABSOLUTE\n");
              #endif
              }
            else
       if (type  == IMAGE_REL_BASED_HIGH)
              {
               WORD deltaw=(WORD)(delta >>16);
               #ifdef DEBUG_PEMODULE
               printf("IMAGE_REL_BASED_HIGH\n");
               #endif
               *dw+= deltaw;
              }
           else
       if (type == IMAGE_REL_BASED_LOW)
              {
               WORD deltaw=(WORD)(delta&0xFFFF);

               #ifdef DEBUG_PEMODULE
               printf("IMAGE_REL_BASED_LOW\n");
               #endif
					*dw+= deltaw;
              }
           else
       if (type == IMAGE_REL_BASED_HIGHLOW)
              {
               #ifdef DEBUG_PEMODULE
               printf("IMAGE_REL_BASED_HIGHLOW: %s ->",itoa(*dd,t1,16));
               #endif
               *dd+=delta;
               #ifdef DEBUG_PEMODULE
               printf("%s\n",itoa(*dd,t1,16));
               #endif
              }
           else
       if (type == IMAGE_REL_BASED_HIGHADJ)
          {

            int highadj=0;
            WORD index2 = table->TypeOffset[i+1]&0xFFFF;
            highadj = (*dw << 16) | index2;
				highadj+= delta;
            highadj+=0x8000;
            *dw = (WORD) (highadj >> 16);
            //skip next entry
            i++;
          }
            else
          {
          	#ifdef DEBUG_PEMODULE
            printf("Unknown.\n");
            #endif
          };
        // relocinfo++;
      };
     table = (IMAGE_BASE_RELOCATION*) ((DWORD) table + (DWORD) table->SizeOfBlock);
    };
    #ifdef DEBUG_PEMODULE
    printf("Relocation complete\n");
    #endif
 };


//searches a library for a particular function name and then returns its
//address
DWORD searchfxnlib(IMAGE_EXPORT_DIRECTORY *exdir,const char *search_name,DWORD base)
{
   DWORD *fxn_names,*fxn_addr,i;
   fxn_names=(DWORD*)((DWORD)exdir->AddressOfNames+base);
   fxn_addr=(DWORD*)((DWORD)exdir->AddressOfFunctions+base);

   #ifdef MODULE_DEBUG
   printf("search name:%s\n",search_name);
   #endif
   
   for (i=0;i<exdir->NumberOfNames;i++)
     {
        char fxnname[256],temp[255],s2[255];

        #ifdef MODULE_DEBUG
        printf("searching  base %s\n",
        itoa((DWORD)base,temp,16));
        #endif

        conexpname((char*)(fxn_names[i]+base),fxnname);
        conexpname(search_name,s2);

        #ifdef MODULE_DEBUG
        printf("got %s ..\n",fxnname);
        #endif

        if (strcmp(fxnname,s2)==0)
        {

        #ifdef MODULE_DEBUG
        printf("Match found!\n");
        printf("located at address:%s\n",itoa(fxn_addr[i]+base,temp,16));
        pause();
        #endif

        return (DWORD)(fxn_addr[i]+base);
        };
     };
    return 0;
};

//links an imported function to its .DLL
//pass 0 to fxn_name to check if a particular module has
//been loaded.

DWORD patchimportedfunction(const char *fxn_name,char *module)
{
lib_PCB *ptr=firstlibrary;
//search the installed libraries
while (ptr!=0)
  {
  char temp1[255],temp2[255];
  strcpy(temp1,module);
  strcpy(temp2,ptr->name);

  strupr(temp1);strupr(temp2);

  #ifdef MODULE_DEBUG
  printf("%s = %s\n",temp1,temp2);
  #endif

   if (strcmp(temp1,temp2)==0) //we have found the correct module
         {

           #ifdef MODULE_DEBUG
           printf("module found!\n");
           pause();
           #endif

           if (fxn_name==0) return 1;

           return (searchfxnlib(ptr->exporttable,fxn_name,(DWORD)ptr->baseaddress));
         };
     ptr=ptr->next;
  };
 return 0; //failed to find the module
};


void showlibinfo()
{
lib_PCB *ptr=firstlibrary;
char temp[255];
printf("Available libraries loaded in memory:\n");
while (ptr!=0)
    {
      printf("Library Name:     %s   location: %s\n",ptr->name,
      itoa((DWORD)ptr->baseaddress,temp,16));
      ptr=ptr->next;
    };
};


int loadSystemDLL(char *name,char *p)
{
 file_PCB *handle;
 int fsize; vfs_stat filestat;
 int hdl,libid;
 char *buf;
 
 handle=openfilex(name,FILE_READ);
 if (!handle) return -1;
 fstat(handle,&filestat);
 vfs_setbuffer(handle,0,filestat.st_size,FILE_IOFBF);
 //get filesize and allocate memory
 fsize= filestat.st_size;
 buf=(char*)malloc(fsize);
 
 //load the file from the disk 
 fread(buf,fsize,1,handle);
 

 libid = dex32_loader(name,buf,lmodeproc,PE_USERDLL,p,0,getprocessid());

 //done!
 free(buf);

 //close the file
 fclose(handle);
 return libid;
};

//loads a user mode pe module and creates a process to start it
int pe_loadusermodule(
        char *module_name, //the name of the module
        char *pe_image,  //the location of the pe image
        DWORD base, //the desired base address to load the image so
                    //that dex can perform the necessary relocations
        int mode,   //Determines what kind of PE to load, USEREXE, USERDLL, SYSDLL or SYSEXE
        char *p,    //the parameters
        char *workdir, //string contaning the working directory of the program
        PCB386 *parent //the parent PCB for EXEs, the parent pagedirectory for DLLs
        ) //location of image to load
{
   char tmpstr[100];
   DWORD *fxn_names,*fxn_addr;
   int i,ret=0;
   DWORD no_tables,pages;
   DWORD *pagedir,*pg;
   DWORD heapres=0,stackres=0;
   IMAGE_DOS_HEADER *hdr;
   IMAGE_FILE_HEADER *pehdr;
   IMAGE_OPTIONAL_HEADER *opthdr;
   IMAGE_SECTION_HEADER *secthdr;
   IMAGE_EXPORT_DIRECTORY *exdir=0;
   IMAGE_BASE_RELOCATION *basere=0;
   IMAGE_IMPORT_DESCRIPTOR *imp_des=0;
   lib_PCB *libptr;

   DWORD *stackloc,*pd;
   void (*entrypoint)(int,char**)=0;
   void (*libentrypoint)(int,char**)=0;
   void (*libexitpoint)()=0;
   void (*signal)(DWORD,DWORD)=0;
   DWORD *ntsignature;
   //a linked list used to keep track of the virtual addresses used
   //by this process so that it could be freed when the process
   //terminates
   process_mem *memptr=0;
  // printf("dex32_module() called..\n");

   hdr=(IMAGE_DOS_HEADER*)pe_image; //obtain the DOS header

    //verify if this has a MZ header
   if (hdr->e_magic!=0x5A4D) return 0;

   pehdr=(IMAGE_FILE_HEADER*)(pe_image+hdr->e_lfanew+4);
   opthdr = (IMAGE_OPTIONAL_HEADER*)(pe_image+hdr->e_lfanew+4
                               +sizeof(IMAGE_FILE_HEADER));
   secthdr = (IMAGE_SECTION_HEADER*)(pe_image+hdr->e_lfanew+4
                               +sizeof(IMAGE_FILE_HEADER)
                               +sizeof(IMAGE_OPTIONAL_HEADER));
   ntsignature=(DWORD*)(pe_image+hdr->e_lfanew);

   /*validate PE file, check for the signature,
     Yes, dex32 also uses the windows NT signature*/
     
   if (*ntsignature!=IMAGE_NT_SIGNATURE) return 0;


   #ifdef DEBUG_PEMODULE
   printf("PE module loader called..\n");
   #endif
   
   if (opthdr->Magic == 0x10b) printf("PE32 file detected.\n");
      else
   if (opthdr->Magic == 0x20b) 
      {
          printf("PE32+ file detected.\n");    
          return 0;
      }
   else
      return 0;
      
   printf("Size of Heap Commit 0x%x\n", opthdr->SizeOfHeapCommit);
   printf("Size of Heap Reserve 0x%x\n", opthdr->SizeOfHeapReserve);
   //For PE's that create a process only, DLL's and SYS use the
   // heap and stacks of the calling process
   if (mode==PE_USEREXE || mode==PE_SYSEXE)
         {
               //since we are going to make a process out of this PE file
               //we must take the standard steps to creating a process
               //like allocating a new pagedirectory
               pagedir=(DWORD*)mempop(); //obtain a physical address from the memory manager
               
               pg=(DWORD*)getvirtaddress((DWORD)pagedir); /*convert to a virtual address so that
                                                            we could use it here without disabling
                                                            the paging mechanism of the 386*/

               //initialize the new pagedirectory
               memset(pg,0,0x1000);

               //allocate the "commited" user stack, the stack actually goes
               //down and not up
               stackloc=(DWORD*)dex32_commitblock((DWORD)userstackloc-opthdr->SizeOfStackCommit,
               opthdr->SizeOfStackCommit,&pages,
               pagedir,PG_WR | PG_USER);
               
               //take note of this so that we could free it later               
               addmemusage(&memptr,stackloc,pages);
               //allocate the "reserve" user stack
               //this pages don't get a physical memory until it actually gets
               //accessed
               stackres=dex32_reserveblock((DWORD)stackloc-opthdr->SizeOfStackReserve,
               opthdr->SizeOfStackReserve,&pages,pagedir,PG_WR);

               //take note of this so that we could free it later               
               addmemusage(&memptr,stackres,pages);

               //calculate the position of the ESP pointer
               stackloc = userstackloc - 4 ;

               //allocate "commited" user heap
               //unlike the stack, this one goes up
               dex32_commitblock((DWORD)userheap,opthdr->SizeOfHeapCommit,&pages,
                        pagedir,PG_WR | PG_USER );

               //take note of this, so that it wouldn't cause a memory leak later
               addmemusage(&memptr,userheap,pages);

               //calculate the position of the reserved heap
               heapres=userheap+(pages*0x1000);

               //allocate "reserved" user heap
               dex32_reserveblock((DWORD)heapres,
                      opthdr->SizeOfHeapReserve,&pages,pagedir,PG_WR );
               
               //take note of this so that we could free it later               
               addmemusage(&memptr,heapres,pages);

           #ifdef DEBUG_PEMODULE
           printf("Heaps and stacks allocated...\n");
           #endif

         };

    if (mode==PE_USERDLL || mode== PE_SYSDLL)
         {
          pagedir= (DWORD*)pagedir1;
         };
    //allocate space for the PE image, obtain its size from opthdr->SizeOfImage
    //then store the starting block in tmpr->addr so that it could be freed
    //when the application terminates
    dex32_commitblock((DWORD)base,opthdr->SizeOfImage,&pages,
               pagedir, PG_WR | PG_USER);
               
    memset(base,0,opthdr->SizeOfImage);
    
           //take note of this, so that it wouldn't cause a memory leak later
           addmemusage(&memptr,base,pages);    
           
   if (mode==PE_USERDLL || mode == PE_SYSDLL)
         {
            lmodeproc_next = (lib_PCB*)( lmodeproc + pages*0x1000);
         }
   //Now transfer the data from the PE image to the memory space we
   //just created earlier
   for (i=0;i<pehdr->NumberOfSections;i++) {
            char *buf;
            DWORD pgattrib=0; //used to determine the attribute of this section

            #ifdef DEBUG_PEMODULE
            printf("Loading section %s.\n",secthdr->Name);
            #endif

            if (secthdr->Characteristics&ATTB_WRITE) //is this section writable?
                  pgattrib|=PG_WR;


            //apply the necessary protections to the pages
             buf = dex32_setpageattbblock((DWORD)base+secthdr->VirtualAddress,
                       secthdr->SizeOfRawData,&pages,
                       pagedir,pgattrib | PG_WR | PG_USER | PG_PRESENT);


           //if this is a .bss section, don't copy anything
            if (secthdr->PointerToRawData!=0)
                 memcpy(buf,pe_image+secthdr->PointerToRawData,
                   secthdr->SizeOfRawData);
                 else
                {
                 //some Programmers assume that their variables in
                 //the .bss section be initialized to 0,... although it is
                 //bad programing practice to assume initial values, some habits
                 //should be tolerated
                 memset(buf,0,secthdr->SizeOfRawData);
                ;};

            //obtain export data description
            if (strcmp(secthdr->Name,".edata")==0)
            exdir=(IMAGE_EXPORT_DIRECTORY*)buf;

            /*check if it is a relocation section*/
            if (strcmp(secthdr->Name,".reloc")==0)
            {
            basere=(IMAGE_BASE_RELOCATION*)buf;
            no_tables=secthdr->SizeOfRawData;
            };


            //obtain import table description

            if (strcmp(module_name,"kernel32.dll")!=0&&
                strcmp(module_name,"KERNEL32.DLL")!=0&&
                strcmp(module_name,"msvcrt.dll")!=0)
            {
               if (strcmp(secthdr->Name,".idata")==0)
                {
                imp_des = (IMAGE_IMPORT_DESCRIPTOR*)buf;

                };
            };
            //  printf("next section...\n");
            secthdr++;
       };

   printf("sections loaded.\n");    
   if (basere!=0)
           relocate(basere,(DWORD)base,opthdr->ImageBase,no_tables);
   else
      {
            if (opthdr->ImageBase!=base)
            {
            printf("relocation table not present.\n");
            printf("Warning: possible failure.\n");
            };
      };        

   if (imp_des!=0)
      importpatch((IMAGE_IMPORT_DESCRIPTOR*)imp_des,(char*)base,pagedir);   
   //if ".edata" is not defined then locate export information from
   //the optional header
   
   printf("Number of Data directories: %d\n", opthdr->NumberOfRvaAndSizes);
   if (!exdir && EXPORT_TABLE < opthdr->NumberOfRvaAndSizes )
            exdir=opthdr->DataDirectory[EXPORT_TABLE].VirtualAddress;


   if (exdir)
    {
   printf("examining export directory..\n");
   fxn_names=(DWORD*)((DWORD)exdir->AddressOfNames+base);
   fxn_addr=(DWORD*)((DWORD)exdir->AddressOfFunctions+base);


   printf("obtaining DEX specific entrypoints..\n");
   printf("Number of exports - %d\n", exdir->NumberOfNames);
   printf("fxn names address = 0x%x\n", fxn_names);
   printf("fxn addresses     = 0x%x\n", fxn_addr);
   
   //Here we find the dex specific entrypoints
   for (i=0;i<exdir->NumberOfNames;i++)
     {
        char fxnname[256];
        conexpname((char*)(fxn_names[i]+base),fxnname);
        printf("function name: %s\n", fxnname);
        if (mode==PE_USEREXE || mode==PE_SYSEXE)
             {
                if (strcmp(fxnname,"dex32_main")==0)
                entrypoint=(void*)(fxn_addr[i]+base);
             }
              else //for libraries
             {
                if (strcmp(fxnname,"dex32_libmain")==0)
                {
                libentrypoint=(void*)(fxn_addr[i]+base);
                entrypoint=(void*)(fxn_addr[i]+base);
                };
                if (strcmp(fxnname,"dex32_exit")==0)
                {
                libexitpoint=(void*)(fxn_addr[i]+base);
                };
             };

        if (strcmp(fxnname,"dex32_signal")==0)
        signal=(void*)(fxn_addr[i]+base);
     };
   };


   if (mode==PE_USERDLL || mode==PE_SYSDLL)
   {
      //add the library to the list of libraries
   if (firstlibrary==0)
    {
      firstlibrary=(lib_PCB*)malloc(sizeof(lib_PCB));
      firstlibrary->next=0;
      lastlibrary=firstlibrary;
      libptr=firstlibrary;
    }
     else
    {
      libptr = firstlibrary;
      while (libptr->next!=0) libptr=libptr->next;
      libptr->next=(lib_PCB*)malloc(sizeof(lib_PCB));
      libptr=libptr->next;
      libptr->next=0;
    };

      strcpy(libptr->name,(char*)(exdir->Name+lmodeproc));
      libptr->baseaddress=(DWORD*)lmodeproc;
      libptr->exporttable=exdir;
      libptr->memptr=memptr;
      libptr->libentrypoint = libentrypoint;
      libptr->libexit = libexitpoint;
      libptr->id = lib_nextid++;
      libptr->lock = 0;
   //update the next virtual address for the next library
   //this should always be the last
      lmodeproc=lmodeproc_next;


      //call the DEX Specific initialization function
      if (libentrypoint!=0)
      {
		if (module_call_library(libentrypoint, libptr, p)==-1) return 0;
      };
      //return the library ID to the caller
      ret=libptr->id;
   };
   #ifdef DEBUG_PEMODULE
   printf("running program..\n");
   #endif

   if (mode==PE_USEREXE || mode==PE_SYSEXE)
    {
             DWORD flags;
             dex32_stopints(&flags);
             if (entrypoint==0)
             {
                   printf("Calling Standard entrypoint..\n");
                   entrypoint=(void*)(base+opthdr->AddressOfEntryPoint);
             }
                  else
             {
                   printf("Calling dex32_main()..\n");       
             };     
             ret = createprocess(entrypoint,module_name,pagedir,memptr,stackloc,
             opthdr->SizeOfStackCommit,SYSCALL_STACK,signal,p,workdir,parent);
             dex32_freeuserpagetable(pagedir1);
             dex32_restoreints(flags);

           
    };

   return ret;
 };


