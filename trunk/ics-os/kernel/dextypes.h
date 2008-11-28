/*
  Name: dextypes.h  
  Copyright: 
  Author: Joseph Emmanuel DL Dayo   
  Date: 02/03/04 17:42
  Description: Defines some types used by dex and some important virtual memory locations
*/

#ifndef DEXTYPES_H
#define DEXTYPES_H

//The current version of this operating system
#define DEX32_OSVER 0x00000001
#define DEX32_VERSION_STRING "1.01"
/*This virtual memory location points to the physical location 
 of the current page directory. The importance of this vritual memory location
 is that the operating system does not have to disable paging everytime it wants
 to change an entry in a page table, instead it refers to this virtual memory location.
*/
#define SYS_PAGEDIR_VIR  0xFFC00000

/*points to the physical location of the auxillary page table, the auxillary page table
  is the one that stores the virtual to physical memory mappings of these vritual
  memory locations. Yes, this is somewhat like a recursive mapping.*/
#define SYS_PAGEDIR2_VIR 0xFFC01000

//holds the location of the virtual address redirector
#define SYS_PAGEDIR3_VIR 0xFFC02000

//holds the location of the kernel page directory location
//if the page directory is the kernel page directory then
//physical addresses PAGEDIR = KERPDIR
#define SYS_KERPDIR_VIR  0xFFC03000
#define SYS_PAGEDIR4_VIR  0xFFC04000

//Types that are commonly used in this operating system
typedef unsigned short int WORD;
typedef unsigned char BYTE;
typedef unsigned int DWORD;

extern char *dex32_versionstring;

#endif
