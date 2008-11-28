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

#define TRUE 1
#define FALSE 0
#define BOOL int

#define ALLOCSIZE 512
#define CACHESIZE 1000

unsigned int usecache=1;


typedef struct _cache {
    DWORD sectorno;
    DWORD accessed;
    WORD valid;
    WORD dirty;  //Used by the driver to determine if a modified block
                 //has already been written to the disk
    char buf[ALLOCSIZE];
} cache;

cache *cacheptr;  //the pointer to the floppy disk cache

/* drive geometry */
typedef struct DrvGeom {
   BYTE heads;
   BYTE tracks;
   BYTE spt;     /* sectors per track */
} DrvGeom;

/* drive geometries */
#define DG144_HEADS       2     /* heads per drive (1.44M) */
#define DG144_TRACKS     80     /* number of tracks (1.44M) */
#define DG144_SPT        18     /* sectors per track (1.44M) */
#define DG144_GAP3FMT  0x54     /* gap3 while formatting (1.44M) */
#define DG144_GAP3RW   0x1b     /* gap3 while reading/writing (1.44M) */

#define DG168_HEADS       2     /* heads per drive (1.68M) */
#define DG168_TRACKS     80     /* number of tracks (1.68M) */
#define DG168_SPT        21     /* sectors per track (1.68M) */
#define DG168_GAP3FMT  0x0c     /* gap3 while formatting (1.68M) */
#define DG168_GAP3RW   0x1c     /* gap3 while reading/writing (1.68M) */

/* IO ports */
#define FDC_DOR  (0x3f2)   /* Digital Output Register */
#define FDC_MSR  (0x3f4)   /* Main Status Register (input) */
#define FDC_DRS  (0x3f4)   /* Data Rate Select Register (output) */
#define FDC_DATA (0x3f5)   /* Data Register */
#define FDC_DIR  (0x3f7)   /* Digital Input Register (input) */
#define FDC_CCR  (0x3f7)   /* Configuration Control Register (output) */

/* command bytes (these are 765 commands + options such as MFM, etc) */
#define FLOP_CMDSPECIFY (0x03)  /* specify drive timings */
#define FLOP_CMDWRITE   (0xc5)  /* write data (+ MT,MFM) */
#define FLOP_CMDREAD    (0xe6)  /* read data (+ MT,MFM,SK) */
#define FLOP_CMDRECAL   (0x07)  /* recalibrate */
#define FLOP_CMDSENSEI  (0x08)  /* sense interrupt status */
#define FLOP_CMDFORMAT  (0x4d)  /* format track (+ MFM) */
#define FLOP_CMDSEEK    (0x0f)  /* seek track */
#define FLOP_CMDVERSION (0x10)  /* FDC version */

/* globals */
static volatile BOOL done = FALSE;
static BOOL dchange = FALSE;
static BOOL motor = FALSE;
static int mtick = 0;
static volatile int tmout = 0;
static BYTE status[7] = { 0 };
static BYTE statsz = 0;
static BYTE sr0 = 0;
static BYTE fdc_track = 0xff;
static DrvGeom geometry = { DG144_HEADS,DG144_TRACKS,DG144_SPT };

/* used to store hardware definition of DMA channels */
typedef struct DmaChannel {
   BYTE page;     /* page register */
   BYTE offset;   /* offset register */
   BYTE length;   /* length register */
} DmaChannel;

/* definition of DMA channels */
const static DmaChannel dmainfo[] = {
   { 0x87, 0x00, 0x01 },
   { 0x83, 0x02, 0x03 },
   { 0x81, 0x04, 0x05 },
   { 0x82, 0x06, 0x07 }
};


static long tbaddr;    /* physical address of track buffer located below 1M */

int floppy_deviceid=0;
int floppy_sigpriority = 0;
char fdcbuf[513];

void sendbyte(int byte);
void reset(void);
void recalibrate(void);
void flop_initcache();
void invalidatecache();
void freecache(DWORD sectornumber);
int shouldflush();
int flushcache();
int getcand();
int storecache(char *buf,DWORD sectornumber,int dirty);
int getcache(char *buf,DWORD sectornumber,DWORD numblocks);
void flopinit(void);
void sendbyte(int byte);
int getbyte();
BOOL waitfdc(BOOL sensei);
void fdchandler(void);
void fdctimer(void);
void dma_xfer(int channel,long physaddr,int length,BOOL read);
void block2hts(int block,int *head,int *track,int *sector);
void reset(void);
BOOL diskchange(void);
void motoron(void);
void motoroff(void);
void recalibrate(void);
BOOL seek(int track);
BOOL log_disk(DrvGeom *g);
BOOL read_block(int block,BYTE *blockbuff,DWORD numblocks);
BOOL write_block(int block,BYTE *blockbuff,DWORD numblocks);
BOOL fdc_rw(int block,BYTE *blockbuff,BOOL read);
BOOL fdc_rw_hts(int head,int track,int sector,BYTE *blockbuff,BOOL read);
BOOL format_track(BYTE track,DrvGeom *g);
int floppy_install();

