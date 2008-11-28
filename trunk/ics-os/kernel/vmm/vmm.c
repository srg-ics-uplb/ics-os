/*
  Name: vmm.c - The Default Virtual Memory Manager
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 10/03/04 18:06
  Description: This module handles the virtual memory management functions of 
  the operating system.
*/

int vmm_pagein(DWORD memloc)
{
    printf("vmm: function not implemented\n");
};

void vmm_init()
{
    devmgr_vmm me;
    
    memset(&me,0,sizeof(me));
    strcpy(me.hdr.name,"default_vmm");
    strcpy(me.hdr.description,"DEX default Virtual Memory Manager");
    me.hdr.size = sizeof(me);
    me.hdr.type = DEVMGR_VMM_EXTENSION;
    me.vmm_pagein = vmm_pagein;

    devmgr_register((devmgr_generic*) &me);
    extension_override(&me,0);
};
