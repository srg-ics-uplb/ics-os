/* Copyright (C) 1998 K.B. Williams, see COPYING.DJ for details */
/* ============ */
/* qsort.c	*/
/* ============ */
#include <assert.h>

/*
 * This is an implementation of quicksort that partitions the array
 * to be sorted into three segments:
 *
 *     1. items that are less than a designated pivot element
 *     2. items that are equal to the pivot element
 *     3. items that are greater than the pivot element
 *
 * This is known as the 'fat pivot' algorithm.  See Kernighan, B.W.
 * and P.J. Plauger, "Software Tools,"  Addison Weslet, 1976, p. 116.
 *
 *
 * One further refinement is used.  When the  number of elements is
 * less than a preset threshold (manifest constant QMIN), insertion
 * sort is used.  In this implementation, insertion sort is used on
 * each segment rather than waiting until the very end to use it on
 * the entire array.
 */

# if !defined(QMIN)
#	define	QMIN	9
# endif

# if defined(PROFILE)
#define	PRIVATE
# else
#define	PRIVATE	static
# endif

typedef	unsigned long	ULONG;

/* ------------------- */
/* FUNCTION PROTOTYPES */
/* ------------------- */
# undef F
# if defined(__STDC__) || defined(__PROTO__)
#	define	F( P )	P
# else
#	define	F( P )	()
# endif
/* INDENT OFF */

PRIVATE	void	InsertionSort F((ULONG First, ULONG Last));
PRIVATE	void	QuickSort F((ULONG, ULONG));
extern	void	qsort F((void *, size_t, size_t,
			int (*)(const void *, const void *)));

PRIVATE	void	Swap F((char  *, char  *));

PRIVATE	int (*Cmpr) F((const void *, const void *));
# undef F

# if defined TEST
	extern	long	SwapCtr;
#	define	BUMP_SWAP_CTR	++SwapCtr
# else
#	define	BUMP_SWAP_CTR
# endif

#define	GET_PIVOT_ADDR(First, Mid, Last)		\
	(CMPR(Mid, First) > 0) ?			\
	    ((CMPR(First, Last) >= 0)	 ? First :	\
		(CMPR(Last, Mid) >= 0)	 ? Mid	 :	\
					   Last) :	\
	    ((CMPR(Last, First) > 0)	 ? First :	\
		(CMPR(Mid, Last) > 0)	 ? Mid	 :	\
					   Last)

#define ADDR(i) (ULONG)((ULONG)(i)*((ULONG)DataWidth))
#define BASE(i) (MyBase + ADDR((i)-1))
#define DEC(A)	(A -= DataWidth)
#define INC(A)	(A += DataWidth)
#define DATA(i) (*(int *)BASE(i))
#define INDEX(a)   (ULONG)((((a) - MyBase)/DataWidth) + 1)
#define CMPR(a, b) (*Cmpr)(a, b)
#define SWAP(a, b)   Swap (a, b)

/* ---------------------- */
/* Local-Global Variables */
/* ---------------------- */
static	size_t	DataWidth;
static	char	*MyBase;


/* INDENT ON */
/* ==================================================================== */
/* qsort - sort a set of elements by the quicksort method		*/
/* ==================================================================== */
# if defined(__STDC__) || defined(__PROTO__)
void
qsort(void *Base, size_t Num, size_t Width,
    int(*Compare) (const void *, const void *))
# else
void
qsort(Base, Num, Width, Compare)
void   *Base;
size_t	Num;
size_t	Width;
int	(*Compare) ();
# endif
{
    Cmpr = Compare;
    DataWidth = Width;
    MyBase = (char *)Base;

    QuickSort((ULONG) 1, (ULONG) Num);
}
/* ==================================================================== */
/* InsertionSort - executes the Straight Insertion Sort algorithm	*/
/* ==================================================================== */
# if defined(__STDC__) || defined(__PROTO__)
PRIVATE void
InsertionSort(ULONG First, ULONG Last)
# else
PRIVATE void
InsertionSort(First, Last)
ULONG	First;
ULONG	Last;
# endif
{
    ULONG   NextIdx, StartIdx;
    char   *NextAdr, *PrevAdr, *StartAdr;

    StartAdr = BASE(First + 1);
    for (StartIdx = First + 1; StartIdx <= Last; ++StartIdx)
    {
	PrevAdr = StartAdr;
	INC(StartAdr);
	for (NextIdx = StartIdx; NextIdx > 1; --NextIdx)
	{
	    NextAdr = PrevAdr;
	    DEC(PrevAdr);

	    if (0 >= CMPR(PrevAdr, NextAdr))
	    {
		break;
	    }

	    SWAP(PrevAdr, NextAdr);
	}
    }
}
/* ==================================================================== */
/* Partition3 - Partitions target array into lows, Fat Pivots and highs */
/* ==================================================================== */
# if defined(__STDC__) || defined(__PROTO__)
PRIVATE void
Partition3(ULONG First, ULONG Last, ULONG * LeftPivot, ULONG * RightPivot)
# else
PRIVATE void
Partition3(First, Last, LeftPivot, RightPivot)
ULONG	First;
ULONG	Last;
ULONG  *LeftPivot;
ULONG  *RightPivot;
# endif
{
    /* ----------------------------------------------------------- */
    /* Classical Two-way Partitioning with Fat Pivot Determination */
    /* ----------------------------------------------------------- */
    ULONG   Left, Right, Mid;		/* Selection indexes	   */
    short   HiFatPvt = 0;		/* Index of High Fat Pivot */
    char   *FirstAddr;			/* Address of First Item   */
    char   *LeftAddr;			/* Left Selection Item	   */
    char   *MidAddr;			/* Mid Selection Item	   */
    char   *RightAddr;			/* Right Selection Item    */
    char   *PivotAddr;			/* Address of Pivot Item   */
    char   *HiFatPvtAddr = 0;		/* Top Address of Fat Pvt  */
    int     CmprLeft = 0;		/* Left Compare Indicator  */
    int     CmprRight = 0;		/* Right Compare Indicator */
    long    FatPvtCtr = 1;		/* Fat Pivot Counter	   */

    Mid       = (First + Last + 1) >> 1;
    LeftAddr  = BASE((First + Mid) >> 1);
    RightAddr = BASE((Mid + Last) >> 1);
    MidAddr   = BASE(Mid);
    FirstAddr = BASE(First);

    /* ---------------------------- */
    /* Select median pivot element. */
    /* ---------------------------- */
    PivotAddr = GET_PIVOT_ADDR(LeftAddr, MidAddr, RightAddr);

    /* -------------------------------------- */
    /* Put the pivot element in a safe place. */
    /* -------------------------------------- */
    if (FirstAddr != PivotAddr)
    {
	SWAP(FirstAddr, PivotAddr);
    }
    /* ------------------------------- */
    /* Initialize selection variables. */
    /* ------------------------------- */
    PivotAddr = FirstAddr;
    Left = First + 1;
    LeftAddr = BASE(Left);
    Right = Last;
    RightAddr = BASE(Right);

    while (Left <= Right)
    {
	/* ------------------------------------------ */
	/* Search from the Left for an item >= Pivot. */
	/* ------------------------------------------ */
	for (; Left <= Right; ++Left, INC(LeftAddr))
	{
	    CmprLeft = CMPR(LeftAddr, PivotAddr);

	    /* ---------------------------------- */
	    /* Break now if Array(Left) >= Pivot. */
	    /* ---------------------------------- */
	    if (0 <= CmprLeft)
	    {
		break;
	    }
	}
	/* --------------------------------------- */
	/* Search from the Right for Item < Pivot. */
	/* --------------------------------------- */
	for (; Right >= Left; --Right, DEC(RightAddr))
	{
	    CmprRight = CMPR(RightAddr, PivotAddr);

	    /* ------------------------------------ */
	    /* If Item at Right < Pivot, Break Now. */
	    /* ------------------------------------ */
	    if (0 > CmprRight)
	    {
		break;
	    }
	    else if (0 < CmprRight)	/* Have Right Item > Pivot */
	    {
		/* ------------------------------------------ */
		/* If Have Fat Pivot, Move Fat Pivot Element. */
		/* ------------------------------------------ */
		if (HiFatPvt)
		{
		    SWAP(HiFatPvtAddr, RightAddr);
		    DEC(HiFatPvtAddr);
		}
	    }
	    else			/* Have Right Item = Pivot */
	    {
		++FatPvtCtr;		/* Bump Fat Pivot Counter */
		/* ------------------------- */
		/* If Not Yet Set, Set Index */
		/* and Location of Fat Pivot */
		/* ------------------------- */
		if (HiFatPvt == 0)
		{
		    HiFatPvt = 1;
		    HiFatPvtAddr = RightAddr;
		}
	    }
	}
	if (Left < Right)
	{
	    SWAP(LeftAddr, RightAddr);

	    if (0 < CmprLeft)
	    {
		/* ------------------------------- */
		/* Item from Left > Pivot. If Have */
		/* Fat Pivot, move Fat Pivot item. */
		/* ------------------------------- */
		if (HiFatPvt)
		{

		    SWAP(HiFatPvtAddr, RightAddr);
		    DEC(HiFatPvtAddr);
		}
	    }
	    else			/* Have Left Item = Pivot */
	    {
		++FatPvtCtr;		/* Bump Fat Pivot Counter */
		/* ------------------------- */
		/* If Not Yet Set, Set Index */
		/* and Location of Fat Pivot */
		/* ------------------------- */
		if (HiFatPvt == 0)
		{
		    HiFatPvt = 1;
		    HiFatPvtAddr = RightAddr;
		}
	    }
	    /* ---------------------------------------- */
	    /* Increment/Decrement Selection Variables. */
	    /* ---------------------------------------- */
	    ++Left, INC(LeftAddr);
	    --Right, DEC(RightAddr);
	}
	else
	{
	    break;
	}
    }
    /* ------------------------------------------------ */
    /* At this point all elements to the right of Right */
    /* are greater than or equal to the pivot element.	*/
    /* All elements to the left of Right are less.	*/
    /* ------------------------------------------------ */
    *LeftPivot = Right;

    *RightPivot = Right + (FatPvtCtr - 1);

    /* ---------------------------------------- */
    /* Move pivot element to its correct place. */
    /* ---------------------------------------- */
    if (PivotAddr != RightAddr)
    {
	SWAP(PivotAddr, RightAddr);
    }
}
/* ==================================================================== */
/* QuickSort - executes the Quicksort algorithm 			*/
/* ==================================================================== */
# if defined(__STDC__) || defined(__PROTO__)
PRIVATE void
QuickSort(ULONG First, ULONG Last)
# else
PRIVATE void
QuickSort(First, Last)
ULONG	First;
ULONG	Last;
# endif
{
    /* ----------------------------- */
    /* QuickSort with Tail Recursion */
    /* ----------------------------- */
    while (First < Last)
    {
	ULONG	LeftPivot, RightPivot;

	if (Last - First < QMIN)
	{
	    InsertionSort(First, Last);
	    break;
	}
	else
	{
	    Partition3(First, Last, &LeftPivot, &RightPivot);

	    /* ----------------------------- */
	    /* Sort Smaller Partition First. */
	    /* ----------------------------- */

	    if (LeftPivot - First < Last - RightPivot)
	    {
		QuickSort(First, LeftPivot - 1);
		First = RightPivot + 1;
	    }
	    else
	    {
		QuickSort(RightPivot + 1, Last);
		Last = LeftPivot - 1;
	    }
	}
    }
}
/* ==================================================================== */
/* Swap - exchanges two data elements each of DataWidth bytes		*/
/* ==================================================================== */
# if defined(__STDC__) || defined(__PROTO__)
PRIVATE	void
Swap(char  * Elem1, char  * Elem2)
# else
PRIVATE void
Swap(Elem1, Elem2)
char  *Elem1;
char  *Elem2;
# endif
# if !defined(MAX_SWAP)
#	define	MAX_SWAP	256
# endif
{
    BUMP_SWAP_CTR;

    /* ----------------------------- */
    /* Swap in Most Efficient Manner */
    /* ----------------------------- */
    switch ((int)DataWidth)
    {
    case 1:
	{
	    char    OneXfer;
	    OneXfer = *Elem1;
	    *Elem1 = *Elem2;
	    *Elem2 = OneXfer;
	    break;
	}
    case 2:
	{
	    short   TwoXfer;
	    TwoXfer = *(short *) Elem1;
	    *(short *) Elem1 = *(short *) Elem2;
	    *(short *) Elem2 = TwoXfer;
	    break;
	}
    case 4:
	{
	    long    FourXfer;
	    FourXfer = *(long *) Elem1;
	    *(long *) Elem1 = *(long *) Elem2;
	    *(long *) Elem2 = FourXfer;
	    break;
	}
    default:
	{
	    char    TempBuf[MAX_SWAP];
	    size_t  CopySize, XferSize;

	    /* ---------------------------------- */
	    /* Copy as much as possible each pass */
	    /* ---------------------------------- */
	    for (XferSize = DataWidth; 0 < XferSize; XferSize -= CopySize)
	    {
		CopySize = (XferSize < sizeof(TempBuf)) ? XferSize :
		    sizeof(TempBuf);

		memcpy(TempBuf, Elem1, CopySize);
		memcpy(Elem1, Elem2, CopySize);
		memcpy(Elem2, TempBuf, CopySize);
		Elem1 += CopySize, Elem2 += CopySize;
	    }
	    break;
	}
    }
}
# if defined(TEST)
#include "tsrtdefs.h"
int
main(int argc, char **argv)
{
    SORT_TEST_STRU SortParms;

    SortParms.argc	= argc;
    SortParms.argv	= argv;
    SortParms.SortFun	= qsort;
    SortParms.OKCodes	= (char *) NULL;
    SortParms.ExecName	= "fqsort";
    SortParms.ExecLabel = "Fat-Pivot Quicksort (Median Pivots)";

    tstsort(&SortParms);
    return 0;
}
# endif
