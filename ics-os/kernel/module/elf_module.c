/*
  Name: elf_module.c ( Extensible and Linkable Format loader)
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 08/04/04 06:58
  Description: Provides limited support for ELF files by interfacing to the
  module loader. There is no relocation support and linking
  support as of yet.
  
  Specifications based on Tools Interface Standard(TIS) version 1.1 for
  Executable and Linkable Format
  
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


/*Defines ELF memory resource sizes, increasing the commit memory
  may improve performance.*/
#define ELF_HEAP_COMMIT 0x10000
#define ELF_HEAP_RESERVE 0x100000
#define ELF_STACK_COMMIT 0x10000
#define ELF_STACK_RESERVE 0x100000

/*Defines if an ELF module is to be loadad as a USER app or SYSTEM app*/
#define ELF_USERO 0
#define ELF_USERSO 1
#define ELF_SYSO  2
#define ELF_SYSSO  3

//defines the machine-Dependent primitive datatypes used by
//the elf standard
   
typedef unsigned int Elf32_Addr;
typedef unsigned short int Elf32_Half;
typedef unsigned int Elf32_Off;
typedef int Elf32_Sword;
typedef unsigned int Elf32_Word;

#define EI_NIDENT 16

// e_type constants
#define ET_NONE 0
//relocatable file
#define ET_REL 1
//executable file
#define ET_EXEC 2
//shared object
#define ET_DYN 3

//reserved values
#define ET_CORE 4
#define ET_LOPROC 0xFF00
#define ET_HIPROC 0xFFFF

//defines the machine type constants
#define EM_NONE 0
#define EM_M32 1
#define EM_SPARC 2
#define EM_386 3
#define EM_68K 4
#define EM_88K 5
#define EM_860 7
#define EM_MIPS 8

//the ELF magic values
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

//section header indexes
#define SHN_UNDEF 0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2
#define SHN_HIRESERVE 0xffff

//section header types
#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11
#define SHT_LOPROC 0x70000000
#define SHT_HIPROC 0x7fffffff
#define SHT_LOUSER 0x80000000
#define SHT_HIUSER 0xffffffff

//program header types
#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7fffffff

//used for interpreting symbol tables
#define ELF32_ST_BIND(i) ((i)>>4)
#define ELF32_ST_TYPE(i) ((i)&0xf)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))

//used for interpreting relocation table information
#define ELF32_R_SYM(i) ((i)>>8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#define ELF32_R_INFO(s,t) (((s)<<8)+(unsigned char)(t))

//symbol types in symbol tables
#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_LOPROC 13
#define STT_HIPROC 15

//Section Attribute flages
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_MASKPROC 0xf0000000

//Intel Architecture Relocation types
#define R_386_GOT32 3 
#define R_386_PLT32 4 
#define R_386_COPY 5 
#define R_386_GLOB_DAT 6 
#define R_386_JMP_SLOT 7 
#define R_386_RELATIVE 8 
#define R_386_GOTOFF 9 
#define R_386_GOTPC 10 

//Dynamic Array Tags
#define DT_NULL 0
#define DT_NEEDED 1
#define DT_PLTRELSZ 2
#define DT_PLTGOT 3
#define DT_HASH 4
#define DT_STRTAB 5
#define DT_SYMTAB 6
#define DT_RELA 7
#define DT_RELASZ 8
#define DT_RELAENT 9
#define DT_STRSZ 10
#define DT_SYMENT 11
#define DT_INIT 12
#define DT_FINI 13
#define DT_SONAME 14
#define DT_RPATH 15
#define DT_SYMBOLIC 16
#define DT_REL 17
#define DT_RELSZ 18
#define DT_RELENT 19
#define DT_RELENT 19
#define DT_PLTREL 20
#define DT_DEBUG 21
#define DT_TEXTREL 22
#define DT_JMPREL 23
#define DT_BIND_NOW 24
#define DT_LOPROC 0x70000000
#define DT_HIPROC 0x7fffffff

//The ELF header
typedef struct __attribute__((packed)) {
unsigned char e_ident[EI_NIDENT];
Elf32_Half e_type;
Elf32_Half e_machine;
Elf32_Word e_version;
Elf32_Addr e_entry;
Elf32_Off  e_phoff;
Elf32_Off  e_shoff;
Elf32_Word e_flags;
Elf32_Half e_ehsize;
Elf32_Half e_phentsize;
Elf32_Half e_phnum;
Elf32_Half e_shentsize;
Elf32_Half e_shnum;
Elf32_Half e_shstrndx;
} Elf32_Ehdr;

//The ELF section header
typedef struct __attribute__((packed)) {
Elf32_Word sh_name;
Elf32_Word sh_type;
Elf32_Word sh_flags;
Elf32_Addr sh_addr;
Elf32_Off sh_offset;
Elf32_Word sh_size;
Elf32_Word sh_link;
Elf32_Word sh_info;
Elf32_Word sh_addralign;
Elf32_Word sh_entsize;
}Elf32_Shdr;

//The ELF program header
typedef struct{
Elf32_Word p_type;
Elf32_Off p_offset;
Elf32_Addr p_vaddr;
Elf32_Addr p_paddr ;
Elf32_Word p_filesz ;
Elf32_Word p_memsz;
Elf32_Word p_flags;
Elf32_Word p_align;
} Elf32_Phdr;

//The ELF relocation information
typedef struct {
Elf32_Addr r_offset;
Elf32_Word r_info;
} Elf32_Rel;

typedef struct {
Elf32_Addr r_offset;
Elf32_Word r_info;
Elf32_Sword r_addend;
} Elf32_Rela;

//This structure defines an ELF sysmbol table. Documentation was lifted from the
//Tool Interface Standard (TIS) Executable and Linking Format (ELF) Specification Version 1.2
typedef struct {
    Elf32_Word st_name; //This member holds an index into the object file's symbol string table, 
                        //which holds the character representations of the symbol names.
                        
    Elf32_Addr st_value;//This member gives the value of the associated symbol. 
                        //Depending on the context,this may be an absolute value, 
                        //an address, and so on; details appear below.
                        
    Elf32_Word st_size; //Many symbols have associated sizes. For example, 
                        //a data object's size is the number of bytes contained 
                        //in the object. This member holds 0 if the symbol has no size or
                        //an unknown size.
                        
    unsigned char st_info; //This member specifies the symbol's type and binding 
                           //attributes. 
                           
    unsigned char st_other; //This member currently holds 0 and has no defined meaning.
    Elf32_Half st_shndx; //Every symbol table entry is "defined'' in relation to some section; 
                         //this member holds the relevant section header table index.
    
} Elf32_Sym;

typedef struct {
    Elf32_Sword d_tag;
    union {
    Elf32_Word d_val;
    Elf32_Addr d_ptr;
    } d_un;
} Elf32_Dyn;

typedef struct {
    Elf32_Word namesz;
    Elf32_Word descsz;
    Elf32_Word type;
    char info[];
} Elf32_Note;

int elf_loadmodule(char *module_name,char *elf_image,
       DWORD base,int mode,char *p,char *workdir,PCB386 *parent)
{
  Elf32_Ehdr *elfheader=0;
  Elf32_Shdr *sectionh=0;
  Elf32_Phdr *programh=0;
  char *strtbl=0, *symstrtbl = 0;
  void (*entrypoint)(int,char**)=0;
  int i,totalsections,totalheaders;
  Elf32_Shdr *sectionvirt[200];
  Elf32_Sym *symtable = 0;
  char temp[255];
  process_mem *memptr = 0;
  DWORD *pagedir,pg,pages;
  DWORD heapres=0,stackres=0;
  DWORD *stackloc,*pd;
  DWORD *sectionbase = base;
  int totalrelocations = 0,totalsymentries = 0;
  unsigned int ret = 0;
  Elf32_Shdr *relsection[20];

  //examine header
  elfheader=(Elf32_Ehdr*) elf_image;

  //identify if the image is an elf
  if ( elfheader->e_ident[0]==ELFMAG0 &&
       elfheader->e_ident[1]==ELFMAG1 &&
       elfheader->e_ident[2]==ELFMAG2 &&
       elfheader->e_ident[3]==ELFMAG3)
    {
        //obtain the program's entry point
        entrypoint=(void*)(elfheader->e_entry);
        sectionh=(Elf32_Shdr*)(elf_image+elfheader->e_shoff);
        programh=(Elf32_Phdr*)(elf_image+elfheader->e_phoff);
        totalsections=elfheader->e_shnum;
        totalheaders=elfheader->e_phnum;



        printf("DEX elf loader:\n");
        printf("entry point: [0x%X], ",elfheader->e_entry);
        printf("number of sections:[%d], ",totalsections);
        printf("number of program headers:[%d]\n",totalheaders);

  
      //pagedir=pagedir1; //allocate space for the pagedirectory
       if (mode == ELF_USERSO || mode == ELF_SYSSO || elfheader->e_type == ET_DYN)
       {
          pagedir = pagedir1;
       }
          else
       {  
       //Allocate a new page directory and initialize it.
          pagedir=(DWORD*)mempop();
          pg=(DWORD*)getvirtaddress((DWORD)pagedir);
          memset(pg,0,0x1000);
       };

    /*If this is an executable module, allocate the heap and
      the stack  */
    if (mode == ELF_USERO || mode == ELF_SYSO || elfheader->e_type == ET_EXEC)
    {
        stackloc=(DWORD*)dex32_commitblock((DWORD)userstackloc-ELF_STACK_COMMIT,
                       ELF_STACK_COMMIT,&pages,
                       pagedir,PG_WR | PG_USER);
                       
        addmemusage(&memptr,stackloc,pages);

        //calculate the position of the reserve stack
        stackres=dex32_reserveblock((DWORD)stackloc-ELF_STACK_RESERVE,
            ELF_STACK_RESERVE, &pages, pagedir, PG_WR);
            
        //store the allocation information so that it could be linked to
        //the process PCB and then be able to free up the memory when app terminates
        addmemusage(&memptr,stackres,pages);
        
        stackloc = userstackloc - 4 ;

        //allocate "commited" user heap
        dex32_commitblock((DWORD)userheap, ELF_HEAP_COMMIT, &pages,
                        pagedir, PG_WR | PG_USER );
                        
        addmemusage(&memptr,userheap,pages);

        //calculate the position of the reserved heap
        heapres=userheap+(pages*0x1000);
        //allocate "reserved" user heap
        dex32_reserveblock((DWORD)heapres,
                        ELF_HEAP_RESERVE, &pages, pagedir, PG_WR );
                        
        addmemusage(&memptr,heapres,pages);                
    };
    

     printf("loading section headers:\n");
     if (sectionh)
     {   
        DWORD section_size = 0, section_base_start = 0xFFFFFFFF,
        section_base_end = 0;
        //obtain the location of the string table from the ELF header   
        strtbl = elf_image + sectionh[elfheader->e_shstrndx].sh_offset; 
        
       for (i=0;i<totalsections;i++)
        {
           if  (sectionh[i].sh_addr!=0)     
           if  (sectionh[i].sh_addr < section_base_start)
               section_base_start = sectionh[i].sh_addr;
               
           if ( (sectionh[i].sh_addr + sectionh[i].sh_size) > section_base_end)
               section_base_end = sectionh[i].sh_addr + sectionh[i].sh_size;
        };
        
        printf("ELF region 0x%x - 0x%x.\n", section_base_start, section_base_end);
        
        dex32_commitblock((DWORD)section_base_start,section_base_end - section_base_start + 0x1000,
                          &pages, pagedir, PG_WR | PG_USER);        
                          
        addmemusage(&memptr,section_base_start,pages);                
        
        //scan the sections, and load them into memory if possible
        for (i=0;i<totalsections;i++)
          {
            void *buf;
            char section_name[255];
            strcpy(section_name, strtbl + sectionh[i].sh_name);
            printf("%s: size [%d], offset [0x%X] load [0x%X].. ",
                    section_name,sectionh[i].sh_size,sectionh[i].sh_offset,sectionh[i].sh_addr);
                    
            
            //check if we have a symbol table                
            if ( sectionh[i].sh_type == SHT_SYMTAB ||
                strcmp(section_name,".symtab")==0 )
              {
                  int i2;          
                  //This is a symbol table, which we might use for linking etc...
                   symtable = elf_image + sectionh[i].sh_offset;
                   
                   //obtain associated string table
                   symstrtbl =  elf_image + sectionh[sectionh[i].sh_link].sh_offset;
                   
                   totalsymentries = (sectionh[i].sh_size / sectionh[i].sh_entsize);
              };  
                
 
         if ( (sectionh[i].sh_flags & SHF_ALLOC) &&
              (elfheader->e_type == ET_EXEC) && (sectionh[i].sh_addr!=0) )
                 {
                      
                       if (sectionh[i].sh_offset && sectionh[i].sh_type != SHT_NOBITS) 
                       {
                           DWORD file_offset = (DWORD)elf_image + (DWORD)sectionh[i].sh_offset;
                           //copy section data to the buffer           
                           memcpy(sectionh[i].sh_addr, file_offset, sectionh[i].sh_size);
                           printf("loaded [0x%X].", file_offset);
                       }
                           else /*a .bss section?*/
                       if (sectionh[i].sh_size!=0)
                       {
                           memset(sectionh[i].sh_addr, 0 , sectionh[i].sh_size);                   
                       };                          
                       

                 }
            printf("\n");
          };
     };     
     
     
     //attempt to interpret the symbol table
     if (symtable!=0)
     {
           int i2;
           for (i2 = 0; i2 < totalsymentries; i2++)
                      {
                           if (symtable[i2].st_name!=0)
                              {
                                  //check if the symbol is defined in this file or not                          
                                  if (symtable[i2].st_shndx==0)
                                  {
                                       printf("External ");                             
                                  };                       
                                                               
                                  printf("symbol [%s]: [0x%X]",
                                       symstrtbl + symtable[i2].st_name,symtable[i2].st_value);                           
                                       
                                  if (ELF32_ST_TYPE(symtable[i2].st_info)==STT_FUNC)
                                     {
                                        printf(" function");
                                     };     
                                  printf("\n");   
                              };                        
                      };
    };
               
     //we are done loading section headers, now we try to load the program headers
     if (programh)
     {
       printf("Loading Program headers ..\n");
       
       
       for (i=0;i<totalheaders;i++)
          {
            void *buf;
            char temp[255];
            
            //This must be a loadable segment
            if (programh[i].p_type == PT_LOAD)
            {
                   DWORD section_base_addr;     
                   printf("virtual address: [0x%X] ",programh[i].p_vaddr);
                   printf("file offset: [0x%X] \n",programh[i].p_offset);           
                   printf("Size in memory : %d ", programh[i].p_memsz);
                   printf("Size in file   : %d \n",programh[i].p_filesz); 

            }
              else
            if (programh[i].p_type == PT_DYNAMIC)  
            {
               /*  printf("Dynamic Section Detected:\n");
                                     printf("virtual address: [%s] ",
                           itoa(programh[i].p_vaddr,temp,16));
                   printf("file offset: [%s] \n",
                           itoa(programh[i].p_offset,temp,16));           
                   printf("Size in memory : %s \n",
                           itoa(programh[i].p_memsz,temp,16));
                   printf("Size in file   : %s \n",
                           itoa(programh[i].p_filesz,temp,16)); */

            }
             else
            if (programh[i].p_type == PT_NOTE) //we have a note section
            {
                   /*Do nothing*/     
            };   
          };
     };     
        

     if (mode == ELF_USERO || mode == ELF_SYSO || elfheader->e_type == ET_EXEC)
        {
         DWORD flags;
         dex32_stopints(&flags);
         printf("executing entrypoint at (0x%x)..\n", (DWORD) entrypoint);
         ret = createprocess(entrypoint,module_name,pagedir,memptr,stackloc,
            1000,SYSCALL_STACK,0,p,workdir,parent);
         dex32_freeuserpagetable(pagedir1);
         dex32_restoreints(flags);
        };    
        
        return ret;
    };
  return 0;
;};
