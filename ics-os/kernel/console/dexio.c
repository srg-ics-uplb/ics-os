/*
  Name: DEX32 std I/O low device functions
  Copyright: 
  Author: Joseph Emmanuel De Luna Dayo
  Date: 23/10/03 02:38
  Description: 
  =================================================================
  -This file contains sets of functions used for accessing the video
   adapter and functions for decoding keyboard keystrokes
  -It also contains the keyboard manager and std I/O functions like
   getch(), getchar().
  
*/
//Dex32 video output functions

//this is responsible for putting a character to the screen
//it also recognizes certain teletype characters like \n and \r

//May 19, 2003 - Added support for virtual consoles

void io_setscroll(int value)
{
    Dex32SetScroll(Dex32GetProcessDevice(),value);
};

unsigned int getx()
 {
 #ifdef USE_CONSOLEDDL
     return Dex32GetX(Dex32GetProcessDevice());
 #else
     return CsrX;
 #endif    
 };

unsigned int gety()
 {
 #ifdef USE_CONSOLEDDL
     return Dex32GetY(Dex32GetProcessDevice());
 #else
     return CsrY;
 #endif
 };

void setx(BYTE x)
 {
 #ifdef USE_CONSOLEDDL
     Dex32SetX(Dex32GetProcessDevice(),x);
 #else
     CsrX=x;
 #endif
 };


void sety(BYTE y)
 {
 #ifdef USE_CONSOLEDDL
     Dex32SetY(Dex32GetProcessDevice(),y);
 #else
  	CsrY=y;
 #endif
 };
int gettext(int left, int top, int right, int bottom, void *destin)
{
 Dex32GetText(Dex32GetProcessDevice(),left,top,right,bottom,destin);
};

int puttext(int left, int top, int right, int bottom, void *source)
{
 Dex32PutText(Dex32GetProcessDevice(),left,top,right,bottom,source);
};

void textattr(int newattr)
{
Dex32SetTextAttr(Dex32GetProcessDevice(),newattr);
};

void outputchar(char c)
 {
   if (c=='\n'||c=='\r') {

             //*_CsrX=0;
             CsrX=0;
             nextln();
                          }
       else
       {
        putchar(CsrX,CsrY,c,attb);
        CsrX++;
        putchar(CsrX,CsrY,' ',attb);
        update_cursor(CsrY,CsrX);
       };
    if (CsrX>79) {CsrX=0; nextln();};
 };

void outchar(char x)
 {
   putcEX(x);
 };


void println(char *s,int attbx)
 {
  printf("%s\n",s);
 };

void print(char *s)
 {
   printf(s);
 ;};

void putc(char x)
 {
 #ifdef USE_CONSOLEDDL
  DEX32_DDL_INFO *d = Dex32GetProcessDevice();
  int cx = d->curx ,cy=d->cury;
  char attb = d->attb;
  Dex32PutChar(d,cx,cy,x,attb);
 #else 
  putchar(CsrX,CsrY,x,attb);
 #endif  
 };

void putcEX(char x)
 {
  #ifdef USE_CONSOLEDDL
  Dex32PutC(Dex32GetProcessDevice(),x);
  #else
   
   if (x=='\t') //automatically expand backspace characters
       {
   		int i;
   		for (i=0;i<3;i++)
  	     outputchar(' ');
  		 }
     else
      outputchar(x);
 #endif 
 };

//clears the screen
void clrscr()
 {
 #ifdef USE_CONSOLEDDL
 Dex32Clear(Dex32GetProcessDevice());
 #else
 
 
 memset(0xb8000,0,80*25*2);
 CsrX=0;
 CsrY=0;
 
#endif
 };
 
//the lowest level video memory oepration available aside from
//update_cursor, puthcar places a character with specified attribute or color
//into an X,Y location (supports only textmode color displays as of the
//moment
//-4-9-2003: Added functions for working with virtual consoles
unsigned int putchar(char x,char y,char c,char color)
  {
   #ifdef USE_CONSOLEDDL
   Dex32PutChar(Dex32GetProcessDevice(),x,y,c,color);
   #else
   char *cptr;
   DWORD vidmemloc=0xb8000;
   cptr=(char*)(vidmemloc+ (y * 80 + x) * 2);
   *cptr=c;
   *(cptr+1)=color;
   #endif
  };

int do_printf(const char *fmt, va_list args, fnptr_t fn, void **ptr)
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
				fn(*fmt, ptr);	/* ...just echo it */
				count++;
				break;
			}
/* found %, get next char and advance state to check if next char is a flag */
			state++;
			fmt++;
			/* FALL THROUGH */
/* STATE 1: AWAITING FLAGS (%-0) */
		case 1:
         if (*fmt == '?')
               {
                fmt++;
                if (*fmt=='r') textcolor(RED);
                if (*fmt=='b') textcolor(BLUE);
                if (*fmt=='g') textcolor(GREEN);
                if (*fmt=='w') textcolor(WHITE);
                if (*fmt=='0') textcolor(BLACK);
                count++;
               	state = flags = given_wd = 0;
      				break;
               };
			if(*fmt == '%')	/* %% */
			{
				fn(*fmt, ptr);
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
				actual_wd = strlen(where);
				if(flags & PR_WS)
					actual_wd++;
/* if we pad left with ZEROES, do the sign now */
				if((flags & (PR_WS | PR_LZ)) ==
					(PR_WS | PR_LZ))
				{
					fn('-', ptr);
					count++;
				}
/* pad on left with spaces or zeroes (for right justify) */
EMIT2:				if((flags & PR_LJ) == 0)
				{
					while(given_wd > actual_wd)
					{
						fn(flags & PR_LZ ? '0' :
							' ', ptr);
						count++;
						given_wd--;
					}
				}
/* if we pad left with SPACES, do the sign now */
				if((flags & (PR_WS | PR_LZ)) == PR_WS)
				{
					fn('-', ptr);
					count++;
				}
/* emit string/char/converted number */
				while(*where != '\0')
				{
					fn(*where++, ptr);
					count++;
				}
/* pad on right with spaces (for left justify) */
				if(given_wd < actual_wd)
					given_wd = 0;
				else given_wd -= actual_wd;
				for(; given_wd; given_wd--)
				{
					fn(' ', ptr);
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
/*****************************************************************************
SPRINTF
*****************************************************************************/
int vsprintf_help(unsigned c, void **ptr)
{
	char *dst;

	dst = *ptr;
	*dst++ = c;
	*ptr = dst;
	return 0 ;
}
/*****************************************************************************
*****************************************************************************/
int vsprintf(char *buffer, const char *fmt, va_list args)
{
	int ret_val;

	ret_val = do_printf(fmt, args, vsprintf_help,(void*)& buffer);
	buffer[ret_val] = '\0';
	return ret_val;
}
/*****************************************************************************
*****************************************************************************/
int sprintf(char *buffer, const char *fmt, ...)
{
	va_list args;
	int ret_val;

	va_start(args, fmt);
	ret_val = vsprintf(buffer, fmt, args);
	buffer[ret_val] = '\0';
	va_end(args);
	return ret_val;
}
/*****************************************************************************
PRINTF
You must write your own putchar()
*****************************************************************************/
int vprintf_help(unsigned c, void **ptr)
{
  /* char t[2];
   t[0]=c;
   t[1]=0;
   print(t);*/
   putcEX(c);
	return 0 ;
}
/*****************************************************************************
*****************************************************************************/
int vprintf(const char *fmt, va_list args)
{
	return do_printf(fmt, args, vprintf_help, NULL);
}
/*****************************************************************************
*****************************************************************************/
int printf(const char *fmt, ...)
{
	va_list args;
	int ret_val;

	va_start(args, fmt);
	ret_val = vprintf(fmt, args);
	va_end(args);
	return ret_val;
}

int DDLprintf_help(unsigned c, DEX32_DDL_INFO **dev)
{

  /* char t[2];
   t[0]=c;
   t[1]=0;
   print(t);*/
   Dex32PutC(*dev,c);
   return 0 ;
};


int vDDLprintf(DEX32_DDL_INFO **dev,const char *fmt,va_list args)
{
  return do_printf(fmt,args,DDLprintf_help,dev);
};

int DDLprintf(DEX32_DDL_INFO **dev, const char *fmt, ...)
{
	va_list args;
	int ret_val;

	va_start(args, fmt);
	ret_val = vDDLprintf(dev,fmt, args);
	va_end(args);
	return ret_val;
}
/****************************************************************************
****************************************************************************/

void textcolor(unsigned char c)
{
    #ifdef USE_CONSOLEDDL
    Dex32SetTextColor(Dex32GetProcessDevice(),c);
    #else
	attb&=112;
	attb|=c;
	#endif
};


/***************************************************************************
***************************************************************************/
void textbackground(unsigned char c)
{
    #ifdef USE_CONSOLEDDL
    Dex32SetTextBackground(Dex32GetProcessDevice(),c);
    #else
    attb&=-113;
    c=c << 4;
    attb|=c;
    #endif
};

/***************************************************************************
***************************************************************************/
void scrollup()
{
 DWORD vidmemloc=0xb8000;
 memmove((void*)vidmemloc,(void*)vidmemloc+0x000A0,3840);
};

void update_cursor(int y,int x)
{
Dex32MoveCursor(Dex32GetProcessDevice(),y,x);
};

void nextln()
{
Dex32NextLn(Dex32GetProcessDevice());
};

