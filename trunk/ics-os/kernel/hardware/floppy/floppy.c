/*DEX OS 32- floppy disk driver-- ported by Joseph Emmanuel Dayo
  Dec. 31, 2002
  January 3, 2003 - Added Cache and Error management functions
  */

/*
 * fdc.c
 * 
 * floppy controller handler functions
 * 
 * Copyright (C) 1998  Fabian Nunez
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * The author can be reached by email at: fabian@cs.uct.ac.za
 *
 * or by airmail at: Fabian Nunez
 *                   10 Eastbrooke
 *                   Highstead Road
 *                   Rondebosch 7700
 *                   South Africa
 */


int flop_getblocksize()
{
    return 512;
};

void flop_initcache()
 {
  int i;
  cacheptr=(cache*)malloc(CACHESIZE*sizeof(cache));
  //initialize the cache
  for (i=0;i<CACHESIZE;i++)
   {
     cacheptr[i].sectorno=0;
     cacheptr[i].valid=0;
     cacheptr[i].accessed=0;
     cacheptr[i].dirty=0;
   ;};
  };


void invalidatecache()
 {
  int i;
  for (i=0;i<CACHESIZE;i++)
   {
     cacheptr[i].valid=0;
  ;};
};

void freecache(DWORD sectornumber)
 {
  int index;

  for (index=0;index<CACHESIZE;index++)
  {
   if (cacheptr[index].sectorno==sectornumber&&
       cacheptr[index].valid)
          {
            cacheptr[index].valid=0;
          };

   }

 };

int shouldflush()
 {
  int res=0,index;
    for (index=0;index<CACHESIZE;index++)
    {
       if (cacheptr[index].valid&&cacheptr[index].dirty)
            {
             res=1;break;
            };
    }
    return res;

 };
 
void start_priority()
{
disable_taskswitching();
dex32_irqcntl( IRQ_TIMER | IRQ_FDC | IRQ_MOUSE);
};

void stop_priority()
{
dex32_irqcntl(IRQ_TIMER | IRQ_KEYBOARD | IRQ_FDC | IRQ_MOUSE);
enable_taskswitching();
};

int compare_cache (const void *a, const void *b)
{
  cache *da = (cache *) a;
  cache *db = (cache *) b;

  return (da->sectorno > db->sectorno) - (da->sectorno < db->sectorno);
}



int flushcache()
  {
    int res=0,index,block;
    start_priority();
    qsort(cacheptr,CACHESIZE,sizeof(cache),compare_cache);
    for (index=0;index<CACHESIZE;index++)
    {
       if (cacheptr[index].valid&&cacheptr[index].dirty)
            {
   	          int retry=0,i=0;
	      #ifdef DEBUG_FLUSHMGR
  		  printf("dex32_flushmgr: commiting write..\n");
	      #endif

  	          res=0;
  	          for (retry=0;retry<3&&res==0;retry++)
              {
                 #ifdef WRITE_DEBUG
                 printf("writing %d/%d..\n",cacheptr[index].sectorno,CACHESIZE);
                 #endif
                 res = fdc_rw(cacheptr[index].sectorno,cacheptr[index].buf,FALSE);
              };

              if (res==0) res= -1; 
              cacheptr[index].dirty=0;

	      #ifdef DEBUG_FLUSHMGR
		printf("dex32_flushmgr: commit write done..\n");
	      #endif

            };
    };
    stop_priority();
    return res;
  };

int getcand()
 {
   DWORD min=0xFFFFFFFF;
   int index,c=0;
   //obtain the oldest cache slot
   for (index=0;index<CACHESIZE;index++)
     {
       if (cacheptr[index].accessed<min&&!cacheptr[index].dirty)
         {
           min=cacheptr[index].accessed;
           c=index;
         };

     };
   return c;
 };

int storecache(char *buf,DWORD sectornumber,int dirty)
 {
   int index;
   int freeslot = -1;


  for (index=0;index<CACHESIZE;index++)
   {
   if (!cacheptr[index].valid)
          freeslot=index;
   if (cacheptr[index].valid&&cacheptr[index].sectorno==sectornumber)
          {
            //optimzation: If the data to be placed into a slot
            //is the same as the one already in the slot then
            //ignore this command.
            if (memcmp(&cacheptr[index].buf,buf,ALLOCSIZE)!=0)
            {
              memcpy(&cacheptr[index].buf,buf,ALLOCSIZE);
              cacheptr[index].valid=1;
              cacheptr[index].accessed=0;
              cacheptr[index].sectorno=sectornumber;
              cacheptr[index].dirty=dirty;
            };
            return 1;
          };

   };

  if (freeslot!=-1)
            index=freeslot;
          else
            index=getcand();

            if (index==0) return 0;

            memcpy(&cacheptr[index].buf,buf,ALLOCSIZE);
            cacheptr[index].valid=1;
            cacheptr[index].accessed=0;
            cacheptr[index].sectorno=sectornumber;
            cacheptr[index].dirty=dirty;
            return 1;

 };

int getcache(char *buf,DWORD sectornumber, DWORD numblocks)
 {
   int index=0;
  int i, res = 0,ofs = 0;
  for (i=0;i<numblocks;i++)
  {  
    res = 0;
    for (index=0;index<CACHESIZE;index++)
    {
       if (cacheptr[index].valid&&cacheptr[index].sectorno==sectornumber + i)
          {
            memcpy(buf + ofs,cacheptr[index].buf,ALLOCSIZE);
            cacheptr[index].accessed=time_count;
            res = 1; break;
          };
    };
    
    ofs+=512;
    if (res == 0) return 0;
  };
  return 1;
 }; 

/* init driver */
void flopinit(void)
{
   int i;
   print("floppy: Initializing the floppy disk driver..please wait\n");
   /* allocate track buffer (must be located below 1M) */
   tbaddr = (long)fdcbuf;
   reset();

   /* get floppy controller version */
   sendbyte(FLOP_CMDVERSION);
   i = getbyte();
   flop_initcache();
   if (i == 0x80)
     print("floppy: NEC765 controller found\n");
   else
     print("floppy: enhanced controller found\n");
}



/* sendbyte() routine from intel manual */
void sendbyte(int byte)
{
   volatile int msr;
   int tmo;

   for (tmo = 0;tmo < 128*2;tmo++) {
      msr = inportb(FDC_MSR);
      if ((msr & 0xc0) == 0x80) {
	 outportb(FDC_DATA,byte);
	 return;
      }
      inportb(0x80);   /* delay */
   }
}

/* getbyte() routine from intel manual */
int getbyte()
{
   volatile int msr;
   int tmo;

   for (tmo = 0;tmo < 128*2;tmo++) {
      msr = inportb(FDC_MSR);
      if ((msr & 0xd0) == 0xd0) {
	 return inportb(FDC_DATA);
      }
      inportb(0x80);   /* delay */
   }

   return -1;   /* read timeout */
}

/* this waits for FDC command to complete */
BOOL waitfdc(BOOL sensei)
{
   tmout = 100*2;   /* set timeout to 1 second */

   /* wait for IRQ6 handler to signal command finished */
   while (!done && tmout);

   /* read in command result bytes */
   statsz = 0;
   while ((statsz < 7) && (inportb(FDC_MSR) & (1<<4))) {
      status[statsz++] = getbyte();
   }

   if (sensei) {
      /* send a "sense interrupt status" command */
      sendbyte(FLOP_CMDSENSEI);
      sr0 = getbyte();
      fdc_track = getbyte();
   }

   done = FALSE;

   if (!tmout) {
      /* timed out! */
      if (inportb(FDC_DIR) & 0x80)  /* check for diskchange */
	dchange = TRUE;

      return FALSE;
   } else
     return TRUE;
}

/* This is the IRQ6 handler */
void fdchandler(void)
{
   /* signal operation finished */
   done = TRUE;
}

/* This is the timer (int 1ch) handler */
void fdctimer(void)
{
   if (tmout) --tmout;     /* bump timeout */

   if (mtick > 0)
     --mtick;
   else if (!mtick && motor) {
      outportb(FDC_DOR,0x0c);  /* turn off floppy motor */
      motor = FALSE;
   }
}

void dma_xfer(int channel,long physaddr,int length,BOOL read)
{
   long page,offset;
   DWORD flags;

   /* calculate dma page and offset */
   page = physaddr >> 16;
   offset = physaddr & 0xffff;
   length -= 1;  /* with dma, if you want k bytes, you ask for k - 1 */

   dex32_stopints(&flags);  /* disable irq's */
   /* set the mask bit for the channel */
   outportb(0x0a,channel | 4);

   /* clear flipflop */
   outportb(0x0c,0);

   /* set DMA mode (write+single+r/w) */
   outportb(0x0b,(read ? 0x48 : 0x44) + channel);

   /* set DMA page */
   outportb(dmainfo[channel].page,page);

   /* set DMA offset */
   outportb(dmainfo[channel].offset,offset & 0xff);  /* low byte */
   outportb(dmainfo[channel].offset,offset >> 8);    /* high byte */

   /* set DMA length */
   outportb(dmainfo[channel].length,length & 0xff);  /* low byte */
   outportb(dmainfo[channel].length,length >> 8);    /* high byte */

   /* clear DMA mask bit */
   outportb(0x0a,channel);
   dex32_restoreints(flags);  /* enable irq's */
}

/*
 * converts linear block address to head/track/sector
 *
 * blocks are numbered 0..heads*tracks*spt-1
 * blocks 0..spt-1 are serviced by head #0
 * blocks spt..spt*2-1 are serviced by head 1
 *
 * WARNING: garbage in == garbage out
 */

 void block2hts(int block,int *head,int *track,int *sector)
{
   *head = (block % (geometry.spt * geometry.heads)) / (geometry.spt);
   *track = block / (geometry.spt * geometry.heads);
   *sector = block % geometry.spt + 1;
}


/**** disk operations ****/

/* this gets the FDC to a known state */
void reset(void)
{
   /* stop the motor and disable IRQ/DMA */
   outportb(FDC_DOR,0);

   mtick = 0;
   motor = FALSE;

   /* program data rate (500K/s) */
   outportb(FDC_DRS,0);

   /* re-enable interrupts */
   outportb(FDC_DOR,0x0c);

   /* resetting triggered an interrupt - handle it */
   done = TRUE;
   waitfdc(TRUE);

   /* specify drive timings (got these off the BIOS) */
   sendbyte(FLOP_CMDSPECIFY);
   sendbyte(0xdf);  /* SRT = 3ms, HUT = 240ms */
   sendbyte(0x02);  /* HLT = 16ms, ND = 0 */

   /* clear "disk change" status */
   seek(1);
   recalibrate();

   dchange = FALSE;
}

/* this returns whether there was a disk change */
BOOL diskchange(void)
{
   return dchange;
}

/* this turns the motor on */
void motoron(void)
{
   if (!motor) {
      mtick = -1;     /* stop motor kill countdown */
      outportb(FDC_DOR,0x1c);
      delay(300);
      motor = TRUE;
   }
}

/* this turns the motor off */
void motoroff(void)
{
   if (motor) {
      mtick = 400;   /* start motor kill countdown: 36 ticks ~ 2s */
   }
}

/* recalibrate the drive */
void recalibrate(void)
{
   /* turn the motor on */
   motoron();

   /* send actual command bytes */
   sendbyte(FLOP_CMDRECAL);
   sendbyte(0);

   /* wait until seek finished */
   waitfdc(TRUE);

   /* turn the motor off */
   motoroff();
}

/* seek to track */
BOOL seek(int track)
{
   if (fdc_track == track)  /* already there? */
     return TRUE;

   motoron();

   /* send actual command bytes */
   sendbyte(FLOP_CMDSEEK);
   sendbyte(0);
   sendbyte(track);

   /* wait until seek finished */
   if (!waitfdc(TRUE))
     return FALSE;     /* timeout! */

   /* now let head settle for 15ms */
   delay(15);
//   usleep(15000);

   motoroff();

   /* check that seek worked */
   if ((sr0 != 0x20) || (fdc_track != track))
     return FALSE;
   else
     return TRUE;
}

/* checks drive geometry - call this after any disk change */
BOOL log_disk(DrvGeom *g)
{
   /* get drive in a known status before we do anything */
   reset();

   /* assume disk is 1.68M and try and read block #21 on first track */
   geometry.heads = DG168_HEADS;
   geometry.tracks = DG168_TRACKS;
   geometry.spt = DG168_SPT;

   if (read_block(20,NULL,1)) {
      /* disk is a 1.68M disk */
      if (g) {
	 g->heads = geometry.heads;
	 g->tracks = geometry.tracks;
	 g->spt = geometry.spt;
      }
      return TRUE;
   }

   /* OK, not 1.68M - try again for 1.44M reading block #18 on first track */
   geometry.heads = DG144_HEADS;
   geometry.tracks = DG144_TRACKS;
   geometry.spt = DG144_SPT;

   if (read_block(17,NULL,1)) {
     /* disk is a 1.44M disk */
      if (g) {
      	 g->heads = geometry.heads;
	 g->tracks = geometry.tracks;
	 g->spt = geometry.spt;

     }
      return TRUE;
   }
   /* it's not 1.44M or 1.68M - we don't support it */
   return FALSE;
}

/* read block (blockbuff is 512 byte buffer) */
BOOL read_block(int block,BYTE *blockbuff,DWORD numblocks)
{
    int retry=0;
    BOOL res=0;
    int i,ofs = 0;
    char temp[513];
   
    
    if (getcache(blockbuff,block,numblocks)) 
    {return 1;};

    for (i=0;i<numblocks;i++)
    {
       res = 0;
       
       //the driver has a tendency to not work so we
       //retry 3 times in case of failure

       while (retry<3&&res==0)
        {
              if (fdc_rw(block + i ,temp,TRUE))
              {
               if (usecache)
               storecache(temp,block + i,0);
               res=1;
               break;
              };
              retry++;
        };

      if (res==0) {strcpy(scr_debug,"  "); return 0;};
      memcpy(blockbuff + ofs,temp,512);
      ofs += 512;
   
    };

   return res;
}

/*June 22 update: Added support for delayed writes*/
/* write block (blockbuff is a 512 byte buffer) */
BOOL write_block(int block,BYTE *blockbuff, DWORD numblocks)
{
  int res=0;
  int retry=0;
  int i,ofs = 0;
  for (i=0; i<numblocks; i++)
  {
          res=1;
          if (!storecache(blockbuff + ofs, block+ i,1))
          {
                  res=0;
                  for (retry=0;retry<3&&res==0;retry++) 
                  res=fdc_rw(block + i,blockbuff + ofs,FALSE);
          };
          if (res==0) return 0; 
     ofs += 512;
  };
  
   return res;
}

/*
 * since reads and writes differ only by a few lines, this handles both.  This
 * function is called by read_block() and write_block()
 */
BOOL fdc_rw(int block,BYTE *blockbuff,BOOL read)
{
   int head,track,sector,tries,i;
   char temp[255];
   
         
   /* convert logical address into physical address */
   block2hts(block,&head,&track,&sector);
   /* spin up the disk */
      

   if (!read && blockbuff) {
      /* copy data from data buffer into track buffer */
      memcpy(fdcbuf,blockbuff,512);
     // movedata(_my_ds(),(long)blockbuff,_dos_ds,tbaddr,512);
   };

   motoron();
   for (tries = 0;tries < 3;tries++) {
      /* check for diskchange */
     if (inportb(FDC_DIR) & 0x80) {
	 dchange = TRUE;
     motoron();
	 seek(1);  /* clear "disk change" status */
	 recalibrate();
	 motoroff();
     print("disk change ..retry\n");
     if (usecache)
     invalidatecache();
	 return FALSE;
      }

      /* move head to right track */
    for (tries = 0;tries < 4; tries++)
        {
          if (seek(track)) break;
          if (tries == 3)
             { motoroff();
               printf("cannot seek ..\n");
               return FALSE;
             };
        };

      /* send command */
      if (read) {
	 dma_xfer(2,(long)fdcbuf,512,FALSE);
	 sendbyte(FLOP_CMDREAD);
      } else {
	 dma_xfer(2,(long)fdcbuf,512,TRUE);
	 sendbyte(FLOP_CMDWRITE);
      }

      sendbyte(head << 2);
      sendbyte(track);
      sendbyte(head);
      sendbyte(sector);
      sendbyte(2);               /* 512 bytes/sector */
      sendbyte(geometry.spt);
      if (geometry.spt == DG144_SPT)
	sendbyte(DG144_GAP3RW);  /* gap 3 size for 1.44M read/write */
      else
	sendbyte(DG168_GAP3RW);  /* gap 3 size for 1.68M read/write */
      sendbyte(0xff); 
                 /* DTL = unused */

      /* wait for command completion */
      /* read/write don't need "sense interrupt status" */
      if (!waitfdc(FALSE))
      {
        print("floppy: time out!\n");
      	return FALSE;   /* timed out! */
      };

      if ((status[0] & 0xc0) == 0) break;   /* worked! outta here! */

      recalibrate();  /* oops, try again... */
   }

   /* stop the motor */
   motoroff();

   if (read && blockbuff) {
      /* copy data from track buffer into data buffer */
      memcpy(blockbuff,fdcbuf,512);
     
   }
     
   return (tries != 3);
}

/*
 * since reads and writes differ only by a few lines, this handles both.  This
 * function is called by read_block() and write_block()
 */
BOOL fdc_rw_hts(int head,int track,int sector,BYTE *blockbuff,BOOL read)
{
   int tries,i;
   char temp[255];
   /* spin up the disk */
   motoron();

   if (!read && blockbuff) {
      /* copy data from data buffer into track buffer */
      memcpy(fdcbuf,blockbuff,512);
   }

   for (tries = 0;tries < 3;tries++) {
      /* check for diskchange */
      if (inportb(FDC_DIR) & 0x80) {
	     dchange = TRUE;
	     seek(1);  /* clear "disk change" status */
	     recalibrate();
	     motoroff();
 	     return FALSE;
      }

      /* move head to right track */
      if (!seek(track)) {
	 motoroff();
     print("cannot seek...\n");
	 return FALSE;
      }

      /* send command */
      if (read) 
      {
  	      dma_xfer(2,(long)fdcbuf,512,FALSE);
	      sendbyte(FLOP_CMDREAD);
      } 
            else 
      {
          dma_xfer(2,(long)fdcbuf,512,TRUE);
	      sendbyte(FLOP_CMDWRITE);
      };

      sendbyte(head << 2);
      sendbyte(track);
      sendbyte(head);
      sendbyte(sector);
      sendbyte(2);               /* 512 bytes/sector */
      sendbyte(geometry.spt);
      if (geometry.spt == DG144_SPT)
	     sendbyte(DG144_GAP3RW);  /* gap 3 size for 1.44M read/write */
      else
	     sendbyte(DG168_GAP3RW);  /* gap 3 size for 1.68M read/write */
	     
      sendbyte(0xff);            /* DTL = unused */

      /* wait for command completion */
      /* read/write don't need "sense interrupt status" */
      if (!waitfdc(FALSE))
      {
    	return FALSE;   /* timed out! */
      };

      if ((status[0] & 0xc0) == 0) break;   /* worked! outta here! */

      recalibrate();  /* oops, try again... */
   }

   /* stop the motor */
   motoroff();

   if (read && blockbuff) {
      /* copy data from track buffer into data buffer */
      memcpy(blockbuff,fdcbuf,512);
   }
   return (tries != 3);
}

/* this formats a track, given a certain geometry */
BOOL format_track(BYTE track,DrvGeom *g)
{
   int i,h,r,r_id,split;
   BYTE tmpbuff[256];

   /* check geometry */
   if (g->spt != DG144_SPT && g->spt != DG168_SPT)
     return FALSE;

   /* spin up the disk */
   motoron();

   /* program data rate (500K/s) */
   outportb(FDC_CCR,0);

   seek(track);  /* seek to track */

   /* precalc some constants for interleave calculation */
   split = g->spt / 2;
   if (g->spt & 1) split++;

   for (h = 0;h < g->heads;h++) {
      /* for each head... */

      /* check for diskchange */
      if (inportb(FDC_DIR) & 0x80) {
	 dchange = TRUE;
	 seek(1);  /* clear "disk change" status */
	 recalibrate();
	 motoroff();
	 return FALSE;
      }

      i = 0;   /* reset buffer index */
      for (r = 0;r < g->spt;r++) {
	 /* for each sector... */

	 /* calculate 1:2 interleave (seems optimal in my system) */
	 r_id = r / 2 + 1;
	 if (r & 1) r_id += split;

	 /* add some head skew (2 sectors should be enough) */
	 if (h & 1) {
	    r_id -= 2;
	    if (r_id < 1) r_id += g->spt;
	 }

	 /* add some track skew (1/2 a revolution) */
	 if (track & 1) {
	    r_id -= g->spt / 2;
	    if (r_id < 1) r_id += g->spt;
	 }

	 /**** interleave now calculated - sector ID is stored in r_id ****/

	 /* fill in sector ID's */
	 tmpbuff[i++] = track;
	 tmpbuff[i++] = h;
	 tmpbuff[i++] = r_id;
	 tmpbuff[i++] = 2;
      }

      /* copy sector ID's to track buffer */
      memcpy(tmpbuff,fdcbuf,i);
     // movedata(_my_ds(),(long)tmpbuff,_dos_ds,tbaddr,i);

      /* start dma xfer */
      dma_xfer(2,tbaddr,i,TRUE);

      /* prepare "format track" command */
      sendbyte(FLOP_CMDFORMAT);
      sendbyte(h << 2);
      sendbyte(2);
      sendbyte(g->spt);
      if (g->spt == DG144_SPT)
	sendbyte(DG144_GAP3FMT);    /* gap3 size for 1.44M format */
      else
	sendbyte(DG168_GAP3FMT);    /* gap3 size for 1.68M format */
      sendbyte(0);     /* filler byte */

      /* wait for command to finish */
      if (!waitfdc(FALSE))
	return FALSE;

      if (status[0] & 0xc0) {
	 motoroff();
	 return FALSE;
      }
   }

   motoroff();

   return TRUE;
}

int floppy_totalblocks()
{
//return the total blocks of a 1.44MB floppy disk
return 3056;
};


//this functions is used to install the floppy disk driver
int floppy_install(const char *name)
{
devmgr_block_desc floppy_desc;


memset(&floppy_desc,0,sizeof(devmgr_block_desc));

//fill up the driver registration form
floppy_desc.hdr.size = sizeof(devmgr_block_desc);
floppy_desc.hdr.type = DEVMGR_BLOCK;
strcpy(floppy_desc.hdr.name,name);
strcpy(floppy_desc.hdr.description,"Generic Floppy Disk Controller driver");
floppy_desc.read_block = read_block;
floppy_desc.write_block = write_block;
floppy_desc.invalidate_cache = invalidatecache;
floppy_desc.init_device = flopinit;
floppy_desc.flush_device = flushcache;
floppy_desc.getcache = getcache;
floppy_desc.total_blocks = floppy_totalblocks;
floppy_desc.get_block_size = flop_getblocksize;
floppy_deviceid = devmgr_register((devmgr_generic*) &floppy_desc);

//initialize the floppy disk
reset();

//assign the keyboard wrapper to IRQ 6
irq_addhandler(floppy_deviceid,6,fdchandler);

return floppy_deviceid;
};
