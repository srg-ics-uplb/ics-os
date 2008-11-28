/*
  Name: kernel heap management module (high level memory management)
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 05/03/04 06:54
  Description: This module handles calls to malloc, free and realloc, it does not
  actually does the memory allocation but serves as a bridge to the custom
  malloc function that the user wants to use.
*/

/*This functions serves as a brdige to the malloc function, it picks the current
  malloc function being used and then diverts to it, for the actual malloc function
  you have to refer to the module it calls*/
void *malloc(unsigned int size)
{
    DWORD flags,val;
    devmgr_malloc_extension *dev_malloc;
    
    storeflags(&flags);
    stopints();
    
    //Use the default malloc function if there is no other malloc functio available    

    if (auxillary_malloc_base == 0)
    {
     
        val = dlmalloc(size);
 
    }
    else
    {
        dev_malloc = (devmgr_malloc_extension*)extension_table[CURRENT_MALLOC].iface;
        val = bridges_call(dev_malloc,&dev_malloc->malloc,size);
    };   

    restoreflags(flags);
    
    return val;
};

/*This functions serves as a brdige to the realloc function, it picks the current
  realloc function being used and then diverts to it, for the actual malloc function
  you have to refer to the module it calls*/
void *realloc(void *ptr,unsigned int size)
{
    DWORD flags,val;
    devmgr_malloc_extension *dev_malloc;
    
    storeflags(&flags);
    stopints();
      
    //Use the default realloc function if there is no other malloc functio available    
    if (auxillary_malloc_base == 0)
        val = dlrealloc(ptr,size);
    else
    if (ptr < auxillary_malloc_base)
    {
        void *memptr;
    /* The old realloc was called when there is another malloc module installed.
       The solution to this is to allocate the new size of memory in the new malloc module,
       copy the data. Unfortunately, if the new size is greater than the size
       currently used, we have a big problem since we do not know how large or how
       small the old data is, memcpy will most likely overstep the bounds of the
       old data. The solution to this of course, is to not use realloc anywhere
       in the kernel :)*/

        
        dev_malloc = (devmgr_malloc_extension*)extension_table[CURRENT_MALLOC].iface;
        memptr = (void*) bridges_call(dev_malloc,&dev_malloc->malloc,size);
        memcpy(memptr, ptr, size);
        
        //use the old free
        dlfree(ptr);        
    }
    else
    {
        dev_malloc = (devmgr_malloc_extension*)extension_table[CURRENT_MALLOC].iface;
        val = bridges_call(dev_malloc,&dev_malloc->realloc,ptr,size);
    };    
    
    restoreflags(flags);
    return val;
};

/*This functions serves as a brdige to the free function, it picks the current
  free function being used and then diverts to it, for the actual free function
  you have to refer to the module it calls*/
void free(void *ptr)
{
    DWORD flags;
    devmgr_malloc_extension *dev_malloc;
    
    storeflags(&flags);
    stopints();
     
    //Use the default realloc function if there is no other malloc functio available
    if (auxillary_malloc_base == 0 || (DWORD)ptr < auxillary_malloc_base)
    {
        dlfree(ptr);
    }
    else
    {
        dev_malloc = (devmgr_malloc_extension*)extension_table[CURRENT_MALLOC].iface;
        bridges_call(dev_malloc,&dev_malloc->free,ptr);
    };    
    
    restoreflags(flags);
};

void alloc_init(const char *alloc_name)
{
int mydevid;

//Register myself to the extension manager
extension_override(devmgr_getdevicebyname(alloc_name),0);

auxillary_malloc_base = 0;
alloc_ready = 1;
};


