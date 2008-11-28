/*
  Name: malloc function # 3
  Copyright: 
  Author: Joseph Emmanuel Dayo
  Date: 06/03/04 05:13
  Description: This is a first fit malloc implementation, it separates allocated blocks 
  a page apart so that any overflow operation will cause a page fault. This module
  is used for debugging purposes only.
*/

#define DEXMALLOC_SPACE 1

typedef struct _mem_malloc {
DWORD mem_magic; /*0xFF2BAD00 maigc value*/
int mem_size;
int isfree;
struct _mem_malloc *next;
} mem_malloc;

mem_malloc *first_block = 0, *last_block = 0;
DWORD mem_nextptr = 0;


void *dex_malloc(size_t size)
    {
        DWORD page = (size + sizeof(mem_malloc)) / 0x1000 +1;
        mem_malloc *ptr;
        
        if ( (size + sizeof(mem_malloc)) %0x1000 == 0) 
            page = (size+ sizeof(mem_malloc)) / 0x1000;
        
        if (first_block == 0 || mem_nextptr == 0)
            {
                DWORD memloc= sbrk(0x100000);
                first_block = (mem_malloc*) memloc;
                mem_nextptr = memloc+ ( (page+DEXMALLOC_SPACE) * 0x1000 );
                commit(memloc, page);       
                first_block->mem_size = page*0x1000 - sizeof(mem_malloc);
                first_block->mem_magic = 0xFF2BAD00;
                first_block->isfree = 0;
                first_block->next = 0;      
                last_block = first_block;   
                return (void*)(memloc+sizeof(mem_malloc));
            }
        else
            {
            ptr = first_block;
                while (ptr!=0)
                   {
                       if (ptr->isfree && ptr->mem_size > size)
                            {
                             ptr->isfree = 0;
                             return (void*)((DWORD)ptr+sizeof(mem_malloc));                     
                            }; 
                       ptr=ptr->next;
                   };
            };        
        
        last_block->next = mem_nextptr;
        ptr = mem_nextptr;
        
        commit((DWORD) mem_nextptr,page);
        mem_nextptr+= (page+DEXMALLOC_SPACE) * 0x1000;
        
        ptr->mem_size = page*0x1000 - sizeof(mem_malloc);
        ptr->mem_magic = 0xFF2BAD00;
        ptr->isfree = 0;
        ptr->next = 0;      
        last_block =ptr;   
        return (void*)((DWORD)ptr+sizeof(mem_malloc));        
    };
    
void dex_free(void *ptr)
{
    mem_malloc *memptr = (mem_malloc*)((DWORD)ptr - sizeof(mem_malloc));
    
    if (memptr->mem_magic!= 0xFF2BAD00)
        {
           printf("invalid free() called.\n");
           while (1);
           return;
        }
    else
        {
           memptr->isfree = 1;
        };
};    

int dexmalloc_sendmessage(int type,const char *message)
{
mem_malloc *ptr;

if (type == DEVMGR_MESSAGESTR)
       {
              int count = 0;
              ptr = first_block;
                while (ptr!=0)
                   {
                       char temp[20];
                       printf("%s size:%d is_free?:%d\n", itoa(ptr+sizeof(mem_malloc),temp,16), 
                       ptr->mem_size, ptr->isfree);
                       ptr=ptr->next;
                   };
                   return 1;
       };
return -1;
};

void dexmalloc_init()
{
int mydevid;
devmgr_malloc_extension mymalloc;

memset(&mymalloc,0,sizeof(devmgr_malloc_extension));
//fill up all the rquired fields
mymalloc.hdr.size = sizeof(devmgr_malloc_extension);
mymalloc.hdr.type = DEVMGR_MALLOC_EXTENSION;
strcpy(mymalloc.hdr.name,"dex_malloc");
strcpy(mymalloc.hdr.description,"malloc for debugging");
mymalloc.malloc  = dex_malloc;
mymalloc.realloc = 0;
mymalloc.free    = dex_free;
mymalloc.hdr.sendmessage= dexmalloc_sendmessage;
mydevid = devmgr_register(&mymalloc);

};

