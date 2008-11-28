/*
  Name: ports.c
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 30/01/04 06:10
  Description: This module handles the management of hardware ports found in an 
               IBM compatible x86 system
*/
define TOTAL_PORTS 65536

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

/*---------------------------------Function Prototypes Here----------------------------------*/
int  ports_close(DWORD devid, DWORD portnum, BYTE attb);
BYTE ports_getstatus(DWORD portnum);
void ports_init();
int  ports_openport(DWORD devid, DWORD portnum,BYTE attb);
int  ports_read(DWORD devid, DWORD portnum,WORD data,int datasize);
int  ports_setattb(DWORD portnum,BYTE attb);
int  ports_write(DWORD devid, DWORD portnum,WORD data,int datasize);  
unsigned char inportb(unsigned int port);
DWORD inportl(unsigned int port);
void outportl(unsigned int port,unsigned int value);
void outportw(unsigned int port,unsigned int value);
unsigned int inportw(unsigned int port);
void outportb(unsigned int port,unsigned char value);

