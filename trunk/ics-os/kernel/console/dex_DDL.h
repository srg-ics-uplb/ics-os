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
#ifndef DEX_DLL_H
#define DEX_DLL_H

#include "../dextypes.h"

typedef struct _dex32_direct_device_hdl {
DWORD size;           //defines the size of this structure
DWORD type;           //reserved, only the text display is supported for now  
DWORD handle;         //a unique identifier

DWORD pid;            //contains the pid of the process that created this
DWORD buf_size;       //defines the size of the write buffer
DWORD locked;         //determines if the device is locked   
DWORD active;         //dtermines if this DDL is active (Writing on buf_ptr writes on the screen)
DWORD bufmode;        //determines if this DDL is in buffer mode  
char *buf_ptr;        //contains a pointer to the write buffer
char *mem_ptr;        //contains a pointer to the memory buffer
char *hdw_ptr;        //contains a pointer to the hardware buffer
char attb;
DWORD curx,cury,lines;
int scroll;
//added 2/2/2004, support for ANSI escape codes
char ansi_command_ptr;
char ansi_cmd[20];
char ansi_x,ansi_y;
} DEX32_DDL_INFO;



//determines if it is possible to write to the buffer or not
#define DDL_LOCKED 1
#define DDL_CGA 0

extern DEX32_DDL_INFO *ActiveDDL;
extern int totalDDL;

DEX32_DDL_INFO *Dex32CreateDDL();
DEX32_DDL_INFO *Dex32SetActiveDDL(DEX32_DDL_INFO *dev);
void Dex32Clear(DEX32_DDL_INFO *dev);
void Dex32ScrollUp(DEX32_DDL_INFO *dev);
void Dex32SetTextColor(DEX32_DDL_INFO *dev, char color);
void Dex32SetTextBackground(DEX32_DDL_INFO *dev,char color);
void Dex32NextLn(DEX32_DDL_INFO *dev);
void Dex32PutC(DEX32_DDL_INFO *dev,char c);
int  Dex32PutChar(DEX32_DDL_INFO *dev,int x, int y,char c,char color);
int  Dex32FreeDDL(DEX32_DDL_INFO *dev);
int  Dex32GetX(DEX32_DDL_INFO *dev);
int  Dex32GetY(DEX32_DDL_INFO *dev);
void Dex32SetX(DEX32_DDL_INFO *dev,int x);
void Dex32SetY(DEX32_DDL_INFO *dev,int y);
int  Dex32GetAttb(DEX32_DDL_INFO *dev);
void Dex32SetDDL(DEX32_DDL_INFO *dev);
void Dex32MoveCursor(DEX32_DDL_INFO *dev,int y, int x);
int  Dex32GetText(DEX32_DDL_INFO *dev,int left, int top, int right, int bottom, char *destin);
void Dex32SetScroll(DEX32_DDL_INFO *dev, int value);
int Dex32PutText(DEX32_DDL_INFO *dev,int left, int top, int right, int bottom, char *source);
DEX32_DDL_INFO *Dex32SetProcessDDL(DEX32_DDL_INFO *dev, int pid);
DEX32_DDL_INFO *Dex32GetProcessDevice();
void Dex32SetTextAttr(DEX32_DDL_INFO *dev, char attr);
#endif
