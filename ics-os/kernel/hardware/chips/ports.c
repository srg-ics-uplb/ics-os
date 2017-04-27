/*
  Name: ports.c
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 30/01/04 06:10
  Description: This module handles the management of hardware ports found in an 
               IBM compatible x86 system
*/


#define TOTAL_PORTS 65536

//defines the sizes of data
#define PORT_BYTE 0
#define PORT_WORD 1

//defines the  statuses of the ports
#define PORT_UNUSED 0
#define PORT_LOCKREAD 1
#define PORT_LOCKWRITE 2
#define PORT READWRITE 8

BYTE  *portstatus; //the statuses of the ports are used here
DWORD *portinfo;   //the driver ID of the driver using this port


unsigned char inportb(unsigned int port)
{
   unsigned char ret;
   asm volatile ("inb %%dx,%%al":"=a" (ret):"d" (port));
   return ret;
};

DWORD inportl(unsigned int port)
{
   unsigned char ret;
   asm volatile ("inl %%dx,%%eax":"=a" (ret):"d" (port));
   return ret;
};


void outportl(unsigned int port,unsigned int value)
{
  asm volatile ("outl %%eax,%%dx": :"d" (port), "a" (value));
};

/* Output a word to a port */
/* July 6, 2001 added space between :: to make code compatible with gpp */
void outportw(unsigned int port,unsigned int value)
{
   asm volatile ("outw %%ax,%%dx": :"d" (port), "a" (value));
};

unsigned int inportw(unsigned int port)
{
   unsigned int ret;

   asm volatile ("inw %%dx,%%ax":"=a" (ret):"d" (port));
   return ret;
};

/* Output a byte to a port */
/* July 6, 2001 added space between :: to make code compatible with gpp */
void outportb(unsigned int port,unsigned char value)
{
   asm volatile ("outb %%al,%%dx": :"d" (port), "a" (value));
};



int ports_openport(DWORD devid, DWORD portnum,BYTE attb)
  {
    if (portstatus[portnum]!=PORT_UNUSED) return -1; //error , port already open
    portinfo[portnum]=devid;
    return 0;
  };

int ports_close(DWORD devid, DWORD portnum, BYTE attb)
  {
    if (portnum<TOTAL_PORTS) //check for errors
    {
        if (portstatus[portnum]!=PORT_UNUSED) return -1; //error , port already open
        if (portinfo[portnum]!=devid) return -1; //error, you're not my owner
        portinfo[portnum]=PORT_UNUSED;
        return 0;
    };
  };
  
int ports_setattb(DWORD portnum,BYTE attb)
  {
    if (portnum<TOTAL_PORTS) //check for errors
          portstatus[portnum]=attb;
     else
          return -1;
  };

BYTE ports_getstatus(DWORD portnum)
  {
    if (portnum<TOTAL_PORTS)  //check for errors
        return portstatus[portnum];
    return -1; //invalid port number
  };

int ports_write(DWORD devid, DWORD portnum,WORD data,int datasize)
  {
   if (portnum>=TOTAL_PORTS) return -1; //check for errors
   
   if (portstatus[portnum]&PORT_LOCKWRITE)
      {
        if (portinfo[portnum]!=devid)
          return -1; //WRITE LOCK enabled
      };
   if (portstatus[portnum]==PORT_UNUSED)
          return -1 ; //PORT not yet opened
         
   outportw((WORD)portnum,data);
   return 0;
  };

int ports_read(DWORD devid, DWORD portnum,WORD data,int datasize)
  {
  if (portnum>=TOTAL_PORTS) return -1; //check for errors
  if (portstatus[portnum]&PORT_LOCKREAD)
      {
         if (portinfo[portnum]!=devid)
         return -1;
      };
   if (portstatus[portnum]==PORT_UNUSED)
         return -1;
   return inportw((WORD)portnum);         
  };
  
void ports_init()
  {
    int i;
    devmgr_hwportmgr myport;
    

    portstatus=(BYTE*)malloc(TOTAL_PORTS*sizeof(BYTE));

    portinfo=(DWORD*)malloc(TOTAL_PORTS*sizeof(DWORD));
    
    for (i=0;i<TOTAL_PORTS;i++)
      {
            portstatus[i]=PORT_UNUSED;
      };
    
    //register and make it available to the device manager
    myport.hdr.size = sizeof(devmgr_hwportmgr);
    strcpy(myport.hdr.name,"hwportmgr");
    strcpy(myport.hdr.description,"Hardware port manager");
    myport.hdr.type = DEVMGR_PORTMGR;
    myport.ports_close=ports_close;
    myport.ports_getstatus=ports_getstatus;
    myport.ports_openport=ports_openport;
    myport.ports_read=ports_read;
    myport.ports_setattb=ports_setattb;
    myport.ports_write=ports_write;
    devmgr_register((devmgr_generic*)&myport);
        
  ;};


