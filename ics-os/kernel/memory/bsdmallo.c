/*malloc functions obtained from DJGPP*/
/* Joseph Emmanuel Dayo--
 * December 31 ,2002  Removed debugging stuff....
 * made some concurrency adjustments
 * ported for the DEX 32 operating system */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#define size_t unsigned int

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * malloc.c (Caltech) 2/21/82
 * Chris Kingsley, kingsley@cit-20.
 *
 * This is a very fast storage allocator.  It allocates blocks of a small 
 * number of different sizes, and keeps free lists of each size.  Blocks that
 * don't exactly fit are passed up to the next larger size.  In this 
 * implementation, the available sizes are 2^n-4 (or 2^n-10) bytes long.
 * This is designed for use in a virtual memory environment.
 */


#define getpagesize() 4096

/*
 * The overhead on a block is at least 4 bytes.  When free, this space
 * contains a pointer to the next free block, and the bottom two bits must
 * be zero.  When in use, the first byte is set to MAGIC, and the second
 * byte is the size index.  The remaining bytes are for alignment.
 * If range checking is enabled then a second word holds the size of the
 * requested block, less 1, rounded up to a multiple of sizeof(RMAGIC).
 * The order of elements is critical: ov_magic must overlay the low order
 * bits of ov_next, and ov_magic can not be a valid ov_next bit pattern.
 */
union overhead {
  union	overhead *ov_next;	/* when free */
  struct {
    unsigned char	ovu_magic; /* magic number */
    unsigned char	ovu_index; /* bucket # */
#ifdef RCHECK
    unsigned short	ovu_rmagic; /* range magic number */
    unsigned int	ovu_size; /* actual block size */
#endif
  } ovu;
#define	ov_magic	ovu.ovu_magic
#define	ov_index	ovu.ovu_index
#define	ov_rmagic	ovu.ovu_rmagic
#define	ov_size		ovu.ovu_size
};

#define	MAGIC		0xef		/* magic # on accounting info */
#define RMAGIC		0x5555		/* magic # on range info */

#ifdef RCHECK
#define	RSLOP		sizeof (unsigned short)
#else
#define	RSLOP		0
#endif

/*
 * nextf[i] is the pointer to the next free block of size 2^(i+3).  The
 * smallest allocatable block is 8 bytes.  The overhead information
 * precedes the data area returned to the user.
 */
#define	NBUCKETS 30
static	union overhead *nextf[NBUCKETS];

static	int pagesz;			/* page size */
static	int pagebucket;			/* page size bucket */

#ifdef MSTATS
/*
 * nmalloc[i] is the difference between the number of mallocs and frees
 * for a given block size.
 */
static	unsigned int nmalloc[NBUCKETS];
#endif

static void bsdmorecore(int bucket);

#if defined(DEBUG) || defined(RCHECK)
#define	ASSERT(p)   if (!(p)) botch(#p)
static void
botch(char *s)
{
  fprintf(stderr, "\r\nassertion botched: %s\r\n", s);
  (void) fflush(stderr);	/* just in case user buffered it */
  *((int *)-1) = 0;		/* force fault; abort will probably fail if malloc's dead */
  abort();
}
#else
#define	ASSERT(p)
#endif

sync_sharedvar mallocbusy;

int alloc_ready = 0;

void *
bsdmalloc(size_t nbytes)
{
  union overhead *op;
  int bucket, n;
  unsigned amt;
 
  /*
   * First time malloc is called, setup page size and
   * align break pointer so all data will be page aligned.
   */
  if (pagesz == 0) {
    pagesz = n = getpagesize();
    op = (union overhead *)sbrk(0);
    n = n - sizeof (*op) - ((int)op & (n - 1));
    if (n < 0)
      n += pagesz;
    if (n) {
      if (sbrk(n) == (char *)-1)
      {
      	return (NULL);
      };
    }
    bucket = 0;
    amt = 8;
    while (pagesz > amt) {
      amt <<= 1;
      bucket++;
    }
    pagebucket = bucket;
  }
  /*
   * Convert amount of memory requested into closest block size
   * stored in hash buckets which satisfies request.
   * Account for space used per block for accounting.
   */
  if (nbytes <= (n = pagesz - sizeof (*op) - RSLOP)) {
#ifndef RCHECK
    amt = 8;			/* size of first bucket */
    bucket = 0;
#else
    amt = 16;			/* size of first bucket */
    bucket = 1;
#endif
    n = -(sizeof (*op) + RSLOP);
  } else {
    amt = pagesz;
    bucket = pagebucket;
  }
  while (nbytes > amt + n) {
    amt <<= 1;
    if (amt == 0)
      {
       return (NULL);
      };
    bucket++;
  }
  /*
   * If nothing in hash bucket right now,
   * request more memory from the system.
   */
  if ((op = nextf[bucket]) == NULL) {
    bsdmorecore(bucket);
    if ((op = nextf[bucket]) == NULL)
      {
       return (NULL);
      };
  }
  /* remove from linked list */
  nextf[bucket] = op->ov_next;
  op->ov_magic = MAGIC;
  op->ov_index = bucket;
#ifdef MSTATS
  nmalloc[bucket]++;
#endif
#ifdef RCHECK
  /*
   * Record allocated size of block and
   * bound space with magic numbers.
   */
  op->ov_size = (nbytes + RSLOP - 1) & ~(RSLOP - 1);
  op->ov_rmagic = RMAGIC;
  *(unsigned short *)((char *)(op + 1) + op->ov_size) = RMAGIC;
#endif
   return ((char *)(op + 1));
}

/*
 * Allocate more memory to the indicated bucket.
 */
static void
bsdmorecore(int bucket)
{
  union overhead *op;
  int sz;			/* size of desired block */
  int amt;			/* amount to allocate */
  int nblks;			/* how many blocks we get */

  /*
   * sbrk_size <= 0 only for big, FLUFFY, requests (about
   * 2^30 bytes on a VAX, I think) or for a negative arg.
   */
  sz = 1 << (bucket + 3);
#ifdef DEBUG
  ASSERT(sz > 0);
#else
  if (sz <= 0)
    return;
#endif
  if (sz < pagesz) {
    amt = pagesz;
    nblks = amt / sz;
  } else {
    amt = sz + pagesz;
    nblks = 1;
  }
  op = (union overhead *)sbrk(amt);
  /* no more room! */
  if ((int)op == -1)
    return;
  /*
   * Add new memory allocated to that on
   * free list for this hash bucket.
   */
  nextf[bucket] = op;
  while (--nblks > 0) {
    op->ov_next = (union overhead *)((char *)op + sz);
    op = (union overhead *)((char *)op + sz);
  }
  op->ov_next = 0;
}

int freebusy=0;

void
bsdfree(void *cp)
{
  int size;
  union overhead *op;
 
  if (cp == NULL)
   {freebusy=0; return;};
  op = (union overhead *)((char *)cp - sizeof (union overhead));
#ifdef DEBUG
  ASSERT(op->ov_magic == MAGIC); /* make sure it was in use */
#else
  if (op->ov_magic != MAGIC)
    {return;};			/* sanity */
#endif
#ifdef RCHECK
  ASSERT(op->ov_rmagic == RMAGIC);
  ASSERT(*(unsigned short *)((char *)(op + 1) + op->ov_size) == RMAGIC);
#endif
  size = op->ov_index;
  ASSERT(size < NBUCKETS);
  op->ov_next = nextf[size];	/* also clobbers ov_magic */
  nextf[size] = op;
#ifdef MSTATS
  nmalloc[size]--;
#endif

}

void *
bsdrealloc(void *cp, size_t nbytes)
{
  unsigned int onb;
  int i;
  union overhead *op;
  char *res;
  int was_alloced = 0;

  if (cp == NULL)
    return (bsdmalloc(nbytes));
  op = (union overhead *)((char *)cp - sizeof (union overhead));
  if (op->ov_magic == MAGIC)
  {
    was_alloced++;
    i = op->ov_index;
  }
  else
  {
    return 0;
  }
  onb = 1 << (i + 3);
  if (onb < pagesz)
    onb -= sizeof (*op) + RSLOP;
  else
    onb += pagesz - sizeof (*op) - RSLOP;
  /* avoid the copy if same size block */
  if (was_alloced) {
    if (i) {
      i = 1 << (i + 2);
      if (i < pagesz)
	i -= sizeof (*op) + RSLOP;
      else
	i += pagesz - sizeof (*op) - RSLOP;
    }
    if (nbytes <= onb && nbytes > i) {
#ifdef RCHECK
      op->ov_size = (nbytes + RSLOP - 1) & ~(RSLOP - 1);
      *(unsigned short *)((char *)(op + 1) + op->ov_size) = RMAGIC;
#endif
      return(cp);
    }
    else
      bsdfree(cp);
  }
  if ((res = bsdmalloc(nbytes)) == NULL)
    return (NULL);
  if (cp != res)
    memcpy(res, cp, (nbytes < onb) ? nbytes : onb);
  return (res);
}

void bsdmalloc_init()
{
int mydevid;
devmgr_malloc_extension mymalloc;

memset(&mymalloc,0,sizeof(devmgr_malloc_extension));
//fill up all the rquired fields
mymalloc.hdr.size = sizeof(devmgr_malloc_extension);
mymalloc.hdr.type = DEVMGR_MALLOC_EXTENSION;
strcpy(mymalloc.hdr.name,"bsd_malloc");
strcpy(mymalloc.hdr.description,"University of California, Berkeley malloc");
mymalloc.malloc  = bsdmalloc;
mymalloc.realloc = bsdrealloc;
mymalloc.free    = bsdfree;
mydevid = devmgr_register(&mymalloc);

};




