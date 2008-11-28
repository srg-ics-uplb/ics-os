//**************************************************************************
//DEX32 Disk drive scheduler
//April 3, 2003 by Joseph Emmanuel Dayo
//This currently implements a very simple first-come-first-served IO queue
//**************************************************************************


#ifndef _IOSCHED_H
#define _IOSCHED_H

//IO request types
#define IO_NONE 0
#define IO_READ 1
#define IO_WRITE 2

//IO request status
#define IO_PENDING 0
#define IO_COMPLETE 1
#define IO_ERROR 2

//defines the maximum number of devices that could be supported
#define MAX_DEVICE 5

#include "../dextypes.h"
#include "../process/sync.h"

typedef struct _IOrequest
 {
 DWORD rID;   //request ID,for identification purposes
 int type;    //determines what kind of operation is to be performed
 int deviceid; //determines which device that will perform the request
 int status;  //a flag indicating if the IO request has been completed or if an error occured
 DWORD time;  //a flag which indicates the time the request was sent
 void *buf;   //pointer to the buffer in which to place the data
 DWORD highblock,lowblock;
 DWORD num_of_blocks; //the number of blocks to read from the starting block
 struct _IOrequest *next,*prev;
 } IOrequest;

//The registers that the io scheduler uses
extern int IOmgr_pause, IOrequest_time, flush_time ;
extern int flush_counter, forceflush;
extern IOrequest *IOjob;
extern IOrequest *cur;
extern int io_flushid, flushing, flushok;
extern sync_sharedvar IOrequest_busy;


DWORD iomgr_init();
DWORD iomgr_flushmgr();
DWORD iomgr_diskmgr();
IOrequest *IOmgr_obtainjob(int deviceid,
    DWORD lblockhigh,DWORD lblocklow /*for optimization*/);
int   dex32_IOcomplete(DWORD handle);
void  dex32_closeIO(DWORD handle);
DWORD dex32_requestIO(int deviceid,int type,DWORD block,DWORD numblocks, void *buf);

#endif
