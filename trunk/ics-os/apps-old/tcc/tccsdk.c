/*
  Name: tcc built-in DEX-OS SDK
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 12/04/04 05:38
  Description: A built-in C library for tcc aka "Tiny C Compiler".
  Compile in tcc using the -c option to create an object file and
  then link it with the main program.
  
  

    Copyright (C) 2004  Joseph Emmanuel Dayo
    
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
    
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
    
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/
#include "dexsdk.h"

//global ANSI variables
int errno;

FILE *stdout = (FILE*)1, *stdin =(FILE*)2, *stderr=(FILE*)1;

/*executes a dex32 systemcall (int 0x30) , implementation almost similar to linux*/
unsigned int dexsdk_systemcall(int function_number,int p1,int p2,
                  int p3,int p4,int p5)
{
    unsigned int return_value;
    __asm__ volatile ("int $0x30" \
        : "=a" (return_value) \
        : "0" ((long)(function_number)),"b" ((long)(p1)),"c" ((long)(p2)), \
          "d" ((long)(p3)),"S" ((long)(p4)),"D" ((long)(p5)) ); \
    return return_value;
};

int dex_exit(int val)
 {
  dexsdk_systemcall(FXN_EXIT,val,0,0,0,0);
 };

void exit (int status)
{
    dex_exit(status);
};

void getparameters(char *buf)
{
    dexsdk_systemcall(0x50,(int)buf,0,0,0,0);
};

void charputc(char c)
{
  dexsdk_systemcall(6,c,0,0,0,0);
};  

/*this strtok is still not thread safe, so be careful!*/
char *strtok(char *s, const char *delim)
{
  const char *spanp;
  int c, sc;
  char *tok;
  static char *last;


  if (s == NULL && (s = last) == NULL)
    return (NULL);

  /*
   * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
   */
 cont:
  c = *s++;
  for (spanp = delim; (sc = *spanp++) != 0;) {
    if (c == sc)
      goto cont;
  }

  if (c == 0) {			/* no non-delimiter characters */
    last = NULL;
    return (NULL);
  }
  tok = s - 1;

  /*
   * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
   * Note that delim must have one NUL; we stop if we see that, too.
   */
  for (;;) {
    c = *s++;
    spanp = delim;
    do {
      if ((sc = *spanp++) == c) {
	if (c == 0)
	  s = NULL;
	else
	  s[-1] = 0;
	last = s;
	return (tok);
      }
    } while (sc != 0);
  }
  /* NOTREACHED */
};

size_t strlen(const char *str)
{
  const char *s;

  if (str == 0)
    return 0;
  for (s = str; *s; ++s);
  return s-str;
};


#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define LARGE	64		/* use 'ABCDEF' instead of 'abcdef' */

static char * number(char * buf, char * end, long long num, int base, int size, int precision, int type)
{
	char c,sign,tmp[66];
	const char *digits;
	const char small_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	const char large_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;

	digits = (type & LARGE) ? large_digits : small_digits;
	if (type & LEFT)
		type &= ~ZEROPAD;
	if (base < 2 || base > 36)
		return 0;
	c = (type & ZEROPAD) ? '0' : ' ';
	sign = 0;
	if (type & SIGN) {
		if (num < 0) {
			sign = '-';
			num = -num;
			size--;
		} else if (type & PLUS) {
			sign = '+';
			size--;
		} else if (type & SPACE) {
			sign = ' ';
			size--;
		}
	}
	if (type & SPECIAL) {
		if (base == 16)
			size -= 2;
		else if (base == 8)
			size--;
	}
	i = 0;
	if (num == 0)
		tmp[i++]='0';
	else while (num != 0)
		tmp[i++] = digits[num/base];
	if (i > precision)
		precision = i;
	size -= precision;
	if (!(type&(ZEROPAD+LEFT))) {
		while(size-->0) {
			if (buf <= end)
				*buf = ' ';
			++buf;
		}
	}
	if (sign) {
		if (buf <= end)
			*buf = sign;
		++buf;
	}
	if (type & SPECIAL) {
		if (base==8) {
			if (buf <= end)
				*buf = '0';
			++buf;
		} else if (base==16) {
			if (buf <= end)
				*buf = '0';
			++buf;
			if (buf <= end)
				*buf = digits[33];
			++buf;
		}
	}
	if (!(type & LEFT)) {
		while (size-- > 0) {
			if (buf <= end)
				*buf = c;
			++buf;
		}
	}
	while (i < precision--) {
		if (buf <= end)
			*buf = '0';
		++buf;
	}
	while (i-- > 0) {
		if (buf <= end)
			*buf = tmp[i];
		++buf;
	}
	while (size-- > 0) {
		if (buf <= end)
			*buf = ' ';
		++buf;
	}
	return buf;
}

int vprintf_help(unsigned c, void **ptr,FILE *f)
{
    charputc(c);
	return 0 ;
}


int vprintf(const char *fmt, va_list args)
{
	return do_printf(fmt, args, vprintf_help,0,NULL);
}

/* flags used in processing format string */
#define		PR_LJ	0x01	/* left justify */
#define		PR_CA	0x02	/* use A-F instead of a-f for hex */
#define		PR_SG	0x04	/* signed numeric conversion (%d vs. %u) */
#define		PR_32	0x08	/* long (32-bit) numeric conversion */
#define		PR_16	0x10	/* short (16-bit) numeric conversion */
#define		PR_WS	0x20	/* PR_SG set and num was < 0 */
#define		PR_LZ	0x40	/* pad left with '0' instead of ' ' */
#define		PR_FP	0x80	/* pointers are far */

/* largest number handled is 2^32-1, lowest radix handled is 8.
2^32-1 in base 8 has 11 digits (add 5 for trailing NUL and for slop) */
#define		PR_BUFLEN	16

int do_printf(const char *fmt, va_list args, fnptr_t fn,FILE *f, void *ptr)
{
	unsigned state, flags, radix, actual_wd, count, given_wd;
	unsigned char *where, buf[PR_BUFLEN];
	long num;

	state = flags = count = given_wd = 0;
/* begin scanning format specifier list */
	for(; *fmt; fmt++)
	{
		switch(state)
		{
/* STATE 0: AWAITING % */
		case 0:
			if(*fmt != '%')	/* not %... */
			{
				fn(*fmt, &ptr,f);	/* ...just echo it */
				count++;
				break;
			}
/* found %, get next char and advance state to check if next char is a flag */
			state++;
			fmt++;
			/* FALL THROUGH */
/* STATE 1: AWAITING FLAGS (%-0) */
		case 1:
     		if(*fmt == '%')	/* %% */
			{
				fn(*fmt, &ptr,f);
				count++;
				state = flags = given_wd = 0;
				break;
			}
			if(*fmt == '-')
			{
				if(flags & PR_LJ)/* %-- is illegal */
					state = flags = given_wd = 0;
				else
					flags |= PR_LJ;
				break;
			}
/* not a flag char: advance state to check if it's field width */
			state++;
/* check now for '%0...' */
			if(*fmt == '0')
			{
				flags |= PR_LZ;
				fmt++;
			}
			/* FALL THROUGH */
/* STATE 2: AWAITING (NUMERIC) FIELD WIDTH */
		case 2:
			if(*fmt >= '0' && *fmt <= '9')
			{
				given_wd = 10 * given_wd +
					(*fmt - '0');
				break;
			}
/* not field width: advance state to check if it's a modifier */
			state++;
			/* FALL THROUGH */
/* STATE 3: AWAITING MODIFIER CHARS (FNlh) */
		case 3:
			if(*fmt == 'F')
			{
				flags |= PR_FP;
				break;
			}
			if(*fmt == 'N')
				break;
			if(*fmt == 'l')
			{
				flags |= PR_32;
				break;
			}
			if(*fmt == 'h')
			{
				flags |= PR_16;
				break;
			}
/* not modifier: advance state to check if it's a conversion char */
			state++;
			/* FALL THROUGH */
/* STATE 4: AWAITING CONVERSION CHARS (Xxpndiuocs) */
		case 4:
			where = buf + PR_BUFLEN - 1;
			*where = '\0';
			switch(*fmt)
			{
			case 'X':
				flags |= PR_CA;
				/* FALL THROUGH */
/* xxx - far pointers (%Fp, %Fn) not yet supported */
			case 'x':
			case 'p':
			case 'n':
				radix = 16;
				goto DO_NUM;
			case 'd':
			case 'i':
				flags |= PR_SG;
				/* FALL THROUGH */
			case 'u':
				radix = 10;
				goto DO_NUM;
			case 'o':
				radix = 8;
/* load the value to be printed. l=long=32 bits: */
DO_NUM:				if(flags & PR_32)
					num = va_arg(args, unsigned long);
/* h=short=16 bits (signed or unsigned) */
				else if(flags & PR_16)
				{
					if(flags & PR_SG)
						num = va_arg(args, int );
					else
						num = va_arg(args, unsigned );
				}
/* no h nor l: sizeof(int) bits (signed or unsigned) */
				else
				{
					if(flags & PR_SG)
						num = va_arg(args, int);
					else
						num = va_arg(args, unsigned int);
				}
/* take care of sign */
				if(flags & PR_SG)
				{
					if(num < 0)
					{
						flags |= PR_WS;
						num = -num;
					}
				}
/* convert binary to octal/decimal/hex ASCII
OK, I found my mistake. The math here is _always_ unsigned */
				do
				{
					unsigned long temp;

					temp = (unsigned long)num % radix;
					where--;
					if(temp < 10)
						*where = temp + '0';
					else if(flags & PR_CA)
						*where = temp - 10 + 'A';
					else
						*where = temp - 10 + 'a';
					num = (unsigned long)num / radix;
				}
				while(num != 0);
				goto EMIT;
			case 'c':
/* disallow pad-left-with-zeroes for %c */
				flags &= ~PR_LZ;
				where--;
				*where = (unsigned char)va_arg(args,
					unsigned );
				actual_wd = 1;
				goto EMIT2;
			case 's':
/* disallow pad-left-with-zeroes for %s */
				flags &= ~PR_LZ;
				where = va_arg(args, unsigned char *);
EMIT:
				actual_wd = strlen((char*)where);
				if(flags & PR_WS)
					actual_wd++;
/* if we pad left with ZEROES, do the sign now */
				if((flags & (PR_WS | PR_LZ)) ==
					(PR_WS | PR_LZ))
				{
					fn('-', &ptr,f);
					count++;
				}
/* pad on left with spaces or zeroes (for right justify) */
EMIT2:				if((flags & PR_LJ) == 0)
				{
					while(given_wd > actual_wd)
					{
						fn(flags & PR_LZ ? '0' :
							' ', &ptr,f);
						count++;
						given_wd--;
					}
				}
/* if we pad left with SPACES, do the sign now */
				if((flags & (PR_WS | PR_LZ)) == PR_WS)
				{
					fn('-', &ptr,f);
					count++;
				}
/* emit string/char/converted number */
				while(*where != '\0')
				{
					fn(*where++,&ptr,f);
					count++;
				}
/* pad on right with spaces (for left justify) */
				if(given_wd < actual_wd)
					given_wd = 0;
				else given_wd -= actual_wd;
				for(; given_wd; given_wd--)
				{
					fn(' ', &ptr,f);
					count++;
				}
				break;
			default:
				break;
			}
		default:
			state = flags = given_wd = 0;
			break;
		}
	}
	return count;
}


int printf(const char *fmt, ...)
{
	va_list args;
	int ret_val;

	va_start(args, fmt);
	ret_val = vprintf(fmt, args);
	va_end(args);
	return ret_val;
}


/********************CRT control functions******************************/

void  clrscr()
{
    dexsdk_systemcall(8,0,0,0,0,0);
};

int wherex(void)
{
  return getx()+1;
};

int wherey(void)
{
 return gety()+1;
};

int getx()
{
   return dexsdk_systemcall(0x35,0,0,0,0,0);
};

int gety()
{
   return dexsdk_systemcall(0x36,0,0,0,0,0);
};

void setx(int x)
{
   dexsdk_systemcall(0x37,x,0,0,0,0);
};

void sety(int y)
{
   dexsdk_systemcall(0x38,y,0,0,0,0); 
};

void textcolor(char val)
{
   dexsdk_systemcall(13,val,0,0,0,0);
};

void textbackground(char val)
{
   dexsdk_systemcall(14,val,0,0,0,0);
};
  
void outc(char x)
{
    dexsdk_systemcall(0x51,x,0,0,0,0);
};

void update_cursor(int x,int y)
{
    dexsdk_systemcall(7,x,y,0,0,0);
};

void gotoxy(int x,int y)
 {
 setx(x-1);sety(y-1);
 update_cursor(y-1,x-1);
;};

/************Heap management functions***/
void *sbrk(int amt)
 {
   return (void*)dexsdk_systemcall(FXN_SBRK,amt,0,0,0,0);
 };

char *buckets[32] = {0};
int bucket2size[32] = {0};

static int
size2bucket(int size)
{
  int rv = 0x1f;
  int bit = ~0x10;
  int i;

  if (size < 4) size = 4;
  size = (size+3)&~3;

  for (i=0; i<5; i++)
  {
    if (bucket2size[rv&bit] >= size)
      rv &= bit;
    bit>>=1;
  }
  return rv;
}

static void
init_buckets(void)
{
  unsigned b;
  for (b=0; b<32; b++)
    bucket2size[b] = (1<<b);
}


void free(void *ptr)
{
  int b;
  if (ptr == 0)
    return;
  b = *(int *)((char *)ptr-4);
  *(char **)ptr = buckets[b];
  buckets[b] =(char*) ptr;
}

void *realloc(void *ptr, size_t size)
{
  char *newptr;
  int oldsize;
  if (ptr == 0)
    return malloc(size);
  oldsize = bucket2size[*(int *)((char *)ptr-4)];
  if (size <= oldsize)
    return ptr;
  newptr = (char *)malloc(size);
  memcpy(newptr, ptr, oldsize);
  free(ptr);
  return newptr;
}

void *malloc(size_t size)
{

  char *rv;
  int b;

  if (bucket2size[0] == 0)
    init_buckets();

  b = size2bucket(size);
  if (buckets[b])
  {
    rv = buckets[b];
    buckets[b] = *(char **)rv;
    return rv;
  }

  size = bucket2size[b]+4;
  rv = (char *)sbrk(size);

  *(int *)rv = b;
  rv += 4;
  return rv;
}


/************String management***********/
void *memmove (void *dst, const void *src,unsigned int count)
{
        void *ret = dst;

        if (dst <= src || (char*)dst >= ((char*)src + count)) {
                while (count--) {
                        *(char*)dst = *(char*)src;
                        dst = (char*)dst + 1;
                        src = (char*)src + 1;
                }
        }
        else {
                /*
                 * Overlapping Buffers
                 * copy from higher addresses to lower addresses
                 */
                dst = (char*)dst + count - 1;
                src = (char*)src + count - 1;

                while (count--) {
                        *(char*)dst = *(char*)src;
                        dst = (char*)dst - 1;
                        src = (char*)src - 1;
                }
        }


        return(ret);
};

void * memset (void *dst,int val,unsigned int count)
{
        void *start = dst;
        while (count--) {
                *(char *)dst = (char)val;
                dst = (char *)dst + 1;
        }
        return(start);
};

void *memchr(const void *s, int c, size_t n)
{
  if (n)
  {
    const char *p = s;
    char cc = c;
    do {
      if (*p == cc)
	return unconst(p, void *);
      p++;
    } while (--n != 0);
  }
  return 0;
}

void * memcpy (void * dst, const void * src,unsigned int count)
{
        void * ret = dst;
        while (count--) {
                *(char *)dst = *(char *)src;
                dst = (char *)dst + 1;
                src = (char *)src + 1;
        }
        return(ret);
};

int memcmp(const void *s1, const void *s2, size_t n)
{
  if (n != 0)
  {
    const unsigned char *p1 = s1, *p2 = s2;

    do {
      if (*p1++ != *p2++)
	return (*--p1 - *--p2);
    } while (--n != 0);
  }
  return 0;
}

char *strcpy(char *to, const char *from)
{
  char *save = to;

  for (; (*to = *from); ++from, ++to);
  return save;
};

char *strchr(const char *s, int c)
{
  char cc = c;
  while (*s)
  {
    if (*s == cc)
      return unconst(s, char *);
    s++;
  }
  if (cc == 0)
    return unconst(s, char *);
  return 0;
}

char *strrchr(const char *s, int c)
{
  char cc = c;
  const char *sp=(char *)0;
  while (*s)
  {
    if (*s == cc)
      sp = s;
    s++;
  }
  if (cc == 0)
    sp = s;
  return unconst(sp, char *);
}

int strcmp(const char *s1, const char *s2)
{
  while (*s1 == *s2)
  {
    if (*s1 == 0)
      return 0;
    s1++;
    s2++;
  }
  return *(unsigned const char *)s1 - *(unsigned const char *)(s2);
}

size_t strcspn(const char *s1, const char *s2)
{
  const char *p, *spanp;
  char c, sc;

  for (p = s1;;)
  {
    c = *p++;
    spanp = s2;
    do {
      if ((sc = *spanp++) == c)
	return p - 1 - s1;
    } while (sc != 0);
  }
  /* NOTREACHED */
}

int strcoll(const char *s1, const char *s2)
{
  return strcmp(s1, s2);
}

size_t strspn(const char *s1, const char *s2)
{
  const char *p = s1, *spanp;
  char c, sc;

 cont:
  c = *p++;
  for (spanp = s2; (sc = *spanp++) != 0;)
    if (sc == c)
      goto cont;
  return (p - 1 - s1);
}

char *strcat(char *s, const char *append)
{
  char *save = s;

  for (; *s; ++s);
  while ((*s++ = *append++));
  return save;
}

char *strstr(const char *s, const char *find)
{
  char c, sc;
  size_t len;

  if ((c = *find++) != 0)
  {
    len = strlen(find);
    do {
      do {
	if ((sc = *s++) == 0)
	  return 0;
      } while (sc != c);
    } while (strncmp(s, find, len) != 0);
    s--;
  }
  return unconst(s, char *);
}

char *strncat(char *dst, const char *src, size_t n)
{
  if (n != 0)
  {
    char *d = dst;
    const char *s = src;
   while (*d != 0)
      d++;
    do {
      if ((*d = *s++) == 0)
	break;
      d++;
    } while (--n != 0);
    *d = 0;
  }
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
  if (n != 0) {
    char *d = dst;
    const char *s = src;

    do {
      if ((*d++ = *s++) == 0)
      {
	while (--n != 0)
	  *d++ = 0;
	break;
      }
    } while (--n != 0);
  }
  return dst;
}

int strncmp(const char *s1, const char *s2, size_t n)
{

  if (n == 0)
    return 0;
  do {
    if (*s1 != *s2++)
      return *(unsigned const char *)s1 - *(unsigned const char *)--s2;
    if (*s1++ == 0)
      break;
  } while (--n != 0);
  return 0;
}

/************Input functions*************/

int kb_deq(int *code)
  {
   return dexsdk_systemcall(1,(int)code,0,0,0,0);
  };
  
char getch()
 {
 int code,c;
   do
   {
    c=kb_deq(&code);
   }
   while (c==-1);
   return ((char)code);
 };

int getchar()
{
  return (int)getch();
};

/******** Files ********/
FILE *openfile(const char  *filename,int mode)
{
  return (FILE*)dexsdk_systemcall(4,(int)filename,mode,0,0,0);
};

int feof(FILE *f)
{
  return dexsdk_systemcall(0x52,(int)f,0,0,0,0);
};

FILE *fopen(const char *filename,const char *s)
 {
   int read=0,write=0,append=0;
   int i;
   for (i=0;s[i];i++)
      {
       if (s[i]=='a') append=1;
       if (s[i]=='w') write=1;
       if (s[i]=='r') read=1;
      };
   if (append) return openfile(filename,FILE_APPEND);
   if (read&&write) return openfile(filename,FILE_READWRITE);
   if (read) return openfile(filename,FILE_READ);
   if (write) return openfile(filename,FILE_WRITE);
   return 0;
};

int fgetc (FILE *stream)
{
   char ch;
   if (stream==stdin) return getchar();
   if (feof(stream))
     return -1;
   fread(&ch,1,1,stream);
   return ch;
};

char *gets(char *buf)
  {
    unsigned int i=0;
    char c;
    do
    {
    c=getch();
    if (c=='\r'||c=='\n'||c==0xa) break;
    if (c=='\b')
       {
       if(i>0)
        {
        i--;

        if (getx()==0)
             {
              setx(79);
              if (gety()>0) sety(gety()-1);
             }
             else
        setx(getx()-1);
        outc(' ');
        };
       }
       else
       {

        if (i<256)  //maximum command line is only 255 characters
         {
            charputc(buf[i]=c);
            i++;
           // setx(getx()+1);

            if (getx()>80) printf("\n");
         };
       };
     outc(' ');
     update_cursor(gety(),getx());
    } while (c!='\r');
    setx(0);
    printf("\n");
    buf[i]=0;
    return buf;
  };

char *fgets(char *s, int n, FILE* f)
{
    
    if (f==stdin)
    {
    int x;
    gets(s);
    x=strlen(s);
    s[x]='\n';
    s[x+1]= 0;
    return s;
    };
    
    return (char*)dexsdk_systemcall(0x40,(int)s,n,(int)f,0,0); 
};

int fread(const void *buf,int itemsize,int noitems,FILE* fhandle)
 {
   return dexsdk_systemcall(0x39,(int)buf,itemsize,noitems,(int)fhandle,0);
 };

int fwrite(const void *buf,int itemsize,int noitems,FILE* fhandle)
 {
   return dexsdk_systemcall(0x45,(int)buf,itemsize,noitems,(int)fhandle,0);
 };
 
char fputc(char c,FILE *f)
  {
    if ( f ==  stdout || f == stderr) //stdout
        charputc(c);
           else
        fwrite(&c,1,1,f);
    return c;
  };

 
int fclose(FILE *stream)
{
 if (stream==stdout) //stdout?
 return 0;
 closefile(stream);
 return 0;
};

int fflush (FILE *stream)
{
    if (stream == stdout || stream==stderr || stream == stdin) return 1;
    return dexsdk_systemcall(0x59,(int)stream,0,0,0,0);
};

char *fseek(FILE* f,long x,int y)
{
   return (char*)dexsdk_systemcall(0x41,(int)f,x,y,0,0);
};

long int ftell(FILE *stream)
{
   return dexsdk_systemcall(0x47,(int)stream,0,0,0,0);
};

int closefile(FILE* fhandle)
{
   return dexsdk_systemcall(5,(int)fhandle,0,0,0,0);
};

int remove(char *filename)
{
    return dexsdk_systemcall(0x49,(int)filename,0,0,0,0);
};

int mkdir (const char *filename, mode_t mode)
{
    return dexsdk_systemcall(0x4A,(int)filename,0,0,0,0);
};

int copyfile(const char *src, const char *dest)
{
    return dexsdk_systemcall(0x97,(int)src,(int)dest,0,0,0);
};


