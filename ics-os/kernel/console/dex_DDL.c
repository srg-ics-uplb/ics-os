/*
  Name: DEX32 Direct Device Layer Management System
  Copyright: 
  Author: Joseph Emmanuel De Luna Dayo
  Date: 23/10/03 02:35
  Description: 
  ==========================================================================
  DEX32 Direct Device Layer Management System
  -This module provides a set of functions for managing output devices
   between multiple processes.
  -It enables each proess to have its own virtual console environment
  ==========================================================================
*/

DEX32_DDL_INFO *ActiveDDL=0;
int totalDDL=0;

DEX32_DDL_INFO *Dex32GetProcessDevice()
{
    if (current_process->outdev == 0 || current_process == 0 ) return consoleDDL;
    return current_process->outdev;
};

DEX32_DDL_INFO *Dex32CreateDDL()
{
    DEX32_DDL_INFO *dev=(DEX32_DDL_INFO*)malloc(sizeof(DEX32_DDL_INFO));
    memset(dev,0,sizeof(DEX32_DDL_INFO));
    dev->size = sizeof(DEX32_DDL_INFO);
    totalDDL++;
    dev->handle=totalDDL;
    dev->buf_size=80*25*2*sizeof(char);
    dev->mem_ptr=(char*)malloc(80*25*2*sizeof(char));    
    memset(dev->mem_ptr,0,80*25*2*sizeof(char));
    dev->buf_ptr=dev->mem_ptr; 
    dev->hdw_ptr=(char*)0xB8000;
    dev->type=DDL_CGA;
    dev->active=0;
    dev->locked=0;
    dev->lines=0;
    dev->scroll = 1;    
    Dex32SetTextColor(dev,WHITE);
    Dex32SetTextBackground(dev,BLACK);
    
    dev->curx=0;dev->cury=0;
    if (ActiveDDL==0) {Dex32SetActiveDDL(dev);};
    
    return dev;
};

DEX32_DDL_INFO *Dex32SetProcessDDL(DEX32_DDL_INFO *dev, int pid)
{
    DEX32_DDL_INFO *ret=0;
    PCB386 *pcb=ps_findprocess(pid);
    
    if (pcb!=-1) /*pid is valid*/
      {
            ret = pcb->outdev;
            pcb->outdev = dev;
            return ret;
      };
    return 0;  
};

void dd_swaptomemory(DEX32_DDL_INFO *dev)
{
memcpy(dev->mem_ptr,dev->hdw_ptr,dev->buf_size);
dev->buf_ptr=dev->mem_ptr;
dev->bufmode = 1;               
};

void dd_swaptohardware(DEX32_DDL_INFO *dev)
{
    if (dev==ActiveDDL)
    {
        memcpy(dev->hdw_ptr,dev->mem_ptr,dev->buf_size);
        dev->buf_ptr=dev->hdw_ptr;    
        dev->bufmode = 0;
    };
};


DEX32_DDL_INFO *Dex32SetActiveDDL(DEX32_DDL_INFO *dev)
{
   DEX32_DDL_INFO *temp_ptr;
   if (dev!=ActiveDDL) //Is dev is already active?
   {
      temp_ptr = ActiveDDL;
      if (ActiveDDL!=0) /*If there is already an activeDLL*/
         {
               //obtain the old active DDL and save the contents in the hardware buffer
               //into the memory buffer
               if (!ActiveDDL->bufmode) 
               {              
                     memcpy(ActiveDDL->mem_ptr,ActiveDDL->hdw_ptr,ActiveDDL->buf_size);
                     ActiveDDL->buf_ptr=ActiveDDL->mem_ptr;
               };
               //Deactive current DDL
               ActiveDDL->active=0;
         };
      
      //copy state of dev to the hardware
      memcpy(dev->hdw_ptr,dev->mem_ptr,dev->buf_size);
      dev->buf_ptr=dev->hdw_ptr;
      ActiveDDL=dev;
      
      //set as the active DDL
      dev->active=1;
      
      if (!dev->bufmode)
      move_cursor(dev->cury,dev->curx);
      return temp_ptr;
   };
   
   return dev;
};


void Dex32Clear(DEX32_DDL_INFO *dev)
{
  memset(dev->buf_ptr,0,dev->buf_size);
  dev->curx=0;dev->cury=0;dev->lines=0;
};

void Dex32ScrollUp(DEX32_DDL_INFO *dev)
{
    DWORD vidmemloc=dev->buf_ptr;
    memmove((void*)vidmemloc,(void*)vidmemloc+0x000A0,3840);

};

void Dex32SetTextAttr(DEX32_DDL_INFO *dev, char attr)
{
dev->attb = attr;
};

void Dex32SetTextColor(DEX32_DDL_INFO *dev, char color)
{
dev->attb&=0xF0;
dev->attb|=color;

};

void Dex32MoveCursor(DEX32_DDL_INFO *dev,int y, int x)
{
    if (dev->active && !dev->bufmode) move_cursor(y,x);
};

void Dex32SetTextBackground(DEX32_DDL_INFO *dev,char color)
{
dev->attb&=0xF;
color=color << 4;
dev->attb|=color;

};

void Dex32SetScroll(DEX32_DDL_INFO *dev, int value)
{
    dev->scroll = value;
};

void Dex32NextLn(DEX32_DDL_INFO *dev)
{
    DWORD vidmemloc=dev->buf_ptr;
    dev->cury++;
    dev->lines++;
    if (dev->cury>=25) 
                        {
              dev->cury=24;
              if (dev->scroll)
                          {
                          Dex32ScrollUp(dev);
                          memset(vidmemloc+0x00F00,0,80*2);
                          };
                       };
                 
    Dex32PutChar(dev,dev->curx,dev->cury,' ',dev->attb);
    if (dev->active && !dev->bufmode) move_cursor(dev->cury,dev->curx);
    
};

void Dex32UpdateCursor(DEX32_DDL_INFO *dev,int y,int x)
{
if (dev->active && !dev->bufmode) move_cursor(y,x);
dev->curx = x;
dev->cury = y;
};


//Emulates an ANSI compatible display subsystem
void Dex32PutC(DEX32_DDL_INFO *dev,char c)
{
       if (c=='\t')
       {
       int i;
              for (i=0;i<3;i++)
              Dex32PutC(dev,' ');
       return;
       }
       else
       if (c=='\b')
             {
             if (dev->curx>0)
             dev->curx--;
             Dex32UpdateCursor(dev,dev->cury,dev->curx);
             }
       else
       if (c=='\r')
             {
             dev->curx=0;
             Dex32UpdateCursor(dev,dev->cury,dev->curx);
             }
       else 
       if (c=='\n')
             {
             dev->curx=0;
             Dex32NextLn(dev);
             }
       else
        {
        Dex32PutChar(dev,dev->curx,dev->cury,c,dev->attb);
        dev->curx++;
        Dex32PutChar(dev,dev->curx,dev->cury,' ',dev->attb);
        
        if (dev->active)        
        Dex32UpdateCursor(dev,dev->cury,dev->curx);
        };
        
    if (dev->curx>79) {dev->curx=0; Dex32NextLn(dev);};
};

int Dex32GetX(DEX32_DDL_INFO *dev)
{
 return dev->curx;
};

int Dex32GetY(DEX32_DDL_INFO *dev)
{
 return dev->cury;
};

void Dex32SetX(DEX32_DDL_INFO *dev,int x)
{
 dev->curx=x;
};

void Dex32SetY(DEX32_DDL_INFO *dev,int y)
{
 dev->cury=y;
};

int Dex32PutChar(DEX32_DDL_INFO *dev,int x, int y,char c,char color)
{
   char *cptr;
   DWORD vidmemloc=dev->buf_ptr;
   
   if (x>=0&&x<80 &&y>=0&&y <25)
   {
   cptr=(char*)(vidmemloc+ (y * 80 + x) * 2);
   *cptr=c;
   *(cptr+1)=color;
   };
};

int Dex32PutText(DEX32_DDL_INFO *dev,int left, int top, int right, int bottom, char *source)
{
   DWORD vidmemloc=dev->buf_ptr;
   int i,i2,i3=0;
  
    for (i2=top;i2<=bottom;i2++)
        for (i=left;i<=right;i++)
       {
             char *cptr;
             cptr=(char*)(vidmemloc+ (i2 * 80 + i) * 2);
             *cptr = source[i3];
             i3++;
             *(cptr+1) = source[i3];
             i3++;
       };
}; 

int Dex32GetText(DEX32_DDL_INFO *dev,int left, int top, int right, int bottom, char *destin)
{
   DWORD vidmemloc=dev->buf_ptr;
   int i,i2,i3=0;
     for (i2=top;i2<=bottom;i2++)
          for (i=left;i<=right;i++)
       {
             char *cptr;
             cptr=(char*)(vidmemloc+ (i2 * 80 + i) * 2);
             destin[i3] = *cptr;
             i3++;
             destin[i3] = *(cptr+1);
             i3++;
       };
}; 

int Dex32GetAttb(DEX32_DDL_INFO *dev)
{
return dev->attb;
};

int Dex32FreeDDL(DEX32_DDL_INFO *dev)
{
    if (ActiveDDL!=dev)
    {
        free(dev);
        return 0;
    };
    return -1;
};

void Dex32SetDDL(DEX32_DDL_INFO *dev)
{
    if (current_process->outdev!=dev)
    {
        current_process->outdev=dev;
    };
    
};
