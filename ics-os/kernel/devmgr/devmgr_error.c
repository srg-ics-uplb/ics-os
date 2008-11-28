/*
  Name: devmgr_error.c (device manager error information module)
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 08/04/04 14:07
  Description: This module provides information about the names of the functions
  in an extension module. This is very useful for debugging
  
*/
#include "dex32_devmgr.h"

const char *get_function_name (int deviceid,int func_num)
{
    devmgr_generic *dev = devmgr_getdevice(deviceid);
    if (dev!=-1)
    {
    if (dev->type == DEVMGR_SCHEDULER_EXTENSION)
        {
              devmgr_scheduler_extension sample_sched;
              DWORD func_index = (DWORD)&sample_sched + sizeof(devmgr_generic) + func_num*4;
              
              if (func_index == (DWORD)&sample_sched.ps_gethead) return "ps_gethead()";
              if (func_index == (DWORD)&sample_sched.ps_enqueue) return "ps_enqueue()";
              if (func_index == (DWORD)&sample_sched.ps_dequeue) return "ps_dequeue()";
              if (func_index == (DWORD)&sample_sched.scheduler) return "scheduler()";
              if (func_index == (DWORD)&sample_sched.ps_listprocess) return "ps_listprocess()";
              if (func_index == (DWORD)&sample_sched.ps_findprocess) return "ps_findprocess()";
        };
     if (dev->type == DEVMGR_FS)
        {
              devmgr_fs_desc sample_fs;
              DWORD func_index =(DWORD)&sample_fs + sizeof(devmgr_generic) + func_num*4;
              
              if (func_index == (DWORD)&sample_fs.identify) return "identify()";
              if (func_index == (DWORD)&sample_fs.mountroot) return "mountroot()";
              if (func_index == (DWORD)&sample_fs.rewritefile) return "rewritefile()";
              if (func_index == (DWORD)&sample_fs.readfile) return "readfile()";
              if (func_index == (DWORD)&sample_fs.chattb) return "chattb()";
              if (func_index == (DWORD)&sample_fs.getsectorsize) return "getsectorsize()";
              if (func_index == (DWORD)&sample_fs.deletefile) return "deletefile()";
              if (func_index == (DWORD)&sample_fs.addsectors) return "addsectors()";
              if (func_index == (DWORD)&sample_fs.writefile) return "writefile()";
              if (func_index == (DWORD)&sample_fs.createfile) return "createfile()";
              if (func_index == (DWORD)&sample_fs.getbytesperblock) return "getbytesperblock()";
              if (func_index == (DWORD)&sample_fs.validate_filename) return "validate_filename()";
              if (func_index == (DWORD)&sample_fs.mountdirectory) return "mountdirectory()";
              if (func_index == (DWORD)&sample_fs.unmount) return "unmount()";
         };
     /*To be completed*/   
     };   
        return "unknown";
};
