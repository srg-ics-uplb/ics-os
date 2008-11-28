/*
  Name: multiboot
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 21/01/04 20:07
  Description: This defines the functions necessary for obtaining information
               from the multiboot header. Multiboot is a specification for bootloaders
               so that it could be provided with information on how to load a particular
               operating system kernel.
*/
#ifndef _MULTIBOOT_H_
#define _MULTIBOOT_H_


#define MB_FLAGS_CMDLINE 4

typedef struct _multiboot_header {
    DWORD flags;
    DWORD mem_lower, mem_upper;
    DWORD boot_device;
    DWORD cmdline;
    DWORD mods_count;
    DWORD mods_addr;
    DWORD syms[4];
    DWORD mmap_length;
    DWORD mmap_addr;
    DWORD drives_length;
    DWORD drives_addr;
    DWORD config_table;
    DWORD boot_loader_name;
    DWORD apm_table;
    DWORD vbe_control_info;
    DWORD vbe_mode_info;
    DWORD vbe_mode;
    DWORD vbe_interface_seg;
    DWORD vbe_interface_off;
    DWORD vbe_interface_len;
} multiboot_header;

typedef struct _mmap {
DWORD   size;
DWORD   base_addr_low;     
DWORD   base_addr_high;
DWORD   length_low;
DWORD   length_high;
DWORD   type;
} mmap;

DWORD map_length = 0;
mmap *memory_map;

#endif
