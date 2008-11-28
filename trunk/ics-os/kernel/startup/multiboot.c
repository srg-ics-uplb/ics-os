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

typedef struct _multiboot_header {
    DWORD flags;
    DWORD mem_lower, mem_upper;
    char boot_device[4];
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
