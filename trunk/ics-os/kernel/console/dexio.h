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

#ifndef __dexio_h_
#define __dexio_h_

PCB386 *foreground;
//defines a structure that would handle a virtual console
//for an application

#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define is_digit(c)	((c) >= '0' && (c) <= '9')

typedef int (*fnptr_t)(unsigned c, void **helper);

#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN 3
#define RED 4
#define MAGENTA 5
#define BROWN 6
#define LIGHTGRAY 7
#define DARKGRAY 8
#define LIGHTBLUE 9
#define LIGHTGREEN 10
#define LIGHTCYAN 11
#define LIGHTRED 12
#define LIGHTMAGENTA 13
#define YELLOW 14
#define WHITE 15
#define BLINK 128



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

//defined in video.asm
//extern void clrscr(void);
//extern void textcolor(unsigned char c);
//extern void textbackground(unsigned char c);
extern void move_cursor(unsigned char row,unsigned int col);

unsigned int putchar(char x,char y,char c,char color);
void nextln();

int sprintf(char * buf, const char *fmt, ...);

//global variables used by the output functions
extern BYTE CsrX;
extern BYTE CsrY;
extern BYTE attb;
//ANSI compatible output mechanism
int esc_mode=0;
char esc_1[3],esc_2[3];



void io_setscroll(int value);
unsigned int getx();
unsigned int gety();
void setx(BYTE x);
void sety(BYTE y);
void outputchar(char c);
void outchar(char x);
void println(char *s,int attbx);
void print(char *s);
void putc(char x);
void putcEX(char x);
void clrscr();
unsigned int putchar(char x,char y,char c,char color);
int do_printf(const char *fmt, va_list args, fnptr_t fn, void **ptr);
int vsprintf_help(unsigned c, void **ptr);
int vsprintf(char *buffer, const char *fmt, va_list args);
int sprintf(char *buffer, const char *fmt, ...);
int vprintf_help(unsigned c, void **ptr);
int vprintf(const char *fmt, va_list args);
int printf(const char *fmt, ...);
int DDLprintf_help(unsigned c, DEX32_DDL_INFO **dev);
int vDDLprintf(DEX32_DDL_INFO **dev,const char *fmt,va_list args);
int DDLprintf(DEX32_DDL_INFO **dev, const char *fmt, ...);
void textcolor(unsigned char c);
void textbackground(unsigned char c);
void update_cursor(int y,int x);
void scrollup();
void nextln();

#endif

