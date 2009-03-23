/*
  Name: dex-os tcc C library
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 12/04/04 21:23
  Description: provides standard C functions to dex-os applications
  compiled using tcc. Some of the functions are based on DJGPP stdlib
  sources.
  
    Copyright (C) 2004 Joseph Dayo

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

#ifndef _TCCSDK_H
#define _TCCSDK_H

/*=============defines SYSTEM CALL constants in DEX==================*/
#define FXN_DEQUEUECHAR 1
#define FXN_FREAD 0x39
#define FXN_FSEEK 0x41
#define FXN_SBRK 0x9
#define FXN_DPUTC 0x6
#define FXN_EXIT 3
#define FXN_SLEEP 0x54
#define FXN_GETPARAMS 0x50
#define FXN_OPENFILE 0x4
#define FXN_TIME 0x55
#define FXN_FSTAT 0x58
#define FXN_CLOSEFILE 0x5
#define FXN_DELFILE 0x59
#define FXN_WRITE 0x45
#define FXN_GETPID 2
#define FXN_GETPPID 27
#define FXN_FINDPROC 28
#define FXN_LOADLIB 29
#define FXN_LIBGETFUNC 30
#define FXN_SETERROR 31
#define FXN_GETERROR 32
#define FXN_SEARCHNAME 33
#define FXN_STAT 36
#define FXN_FORK 0x90
#define FXN_SLEEP 0x54

/*============DEX constants for files===========*/
#define FILE_READ 0
#define FILE_WRITE 1
#define FILE_READWRITE 2
#define FILE_APPEND 3

       /*-----POSIZ Constants-----*/
#define O_RDONLY	0x0000
#define O_WRONLY	0x0001
#define O_RDWR		0x0002
#define O_ACCMODE	0x0003

#define O_BINARY	0x0004	/* must fit in char, reserved by dos */
#define O_TEXT		0x0008	/* must fit in char, reserved by dos */
#define O_NOINHERIT	0x0080	/* DOS-specific */

#define O_CREAT		0x0100	/* second byte, away from DOS bits */
#define O_EXCL		0x0200
#define O_NOCTTY	0x0400
#define O_TRUNC		0x0800
#define O_APPEND	0x1000
#define O_NONBLOCK	0x2000

/*============constants defined in limits.h============*/
#define CHAR_BIT 8
#define CHAR_MAX 127
#define CHAR_MIN (-128)
#define INT_MAX 2147483647
#define INT_MIN (-2147483647-1)
#define LONG_MAX 2147483647L
#define LONG_MIN (-2147483647L-1L)
#define MB_LEN_MAX 5
#define SCHAR_MAX 127
#define SCHAR_MIN (-128)
#define SHRT_MAX 32767
#define SHRT_MIN (-32768)
#define UCHAR_MAX 255
#define UINT_MAX 4294967295U
#define ULONG_MAX 4294967295UL
#define USHRT_MAX 65535
#define WCHAR_MIN 0
#define WCHAR_MAX 127
#define WINT_MIN 0
#define WINT_MAX 32767

/*=============defines color constants, as used by conio.h===========*/

#if !defined(__COLORS)
#define __COLORS

enum COLORS {
    BLACK,          /* dark colors */
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTGRAY,
    DARKGRAY,       /* light colors */
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE
};

#define BLINK 128

#endif
/*============files.h constants=========*/

/*ftell, fseek constants*/
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

#define FILE unsigned int
#define EOF (-1)                /* End of file indicator */

extern FILE *stdout, *stdin, *stderr;

/*============other constants================*/
#ifndef NULL
#define NULL 0
#endif


/* VGA mode constants*/
#define VGA_320X200X256   1
#define VGA_TEXT80X25X16  2
#define VGA_640X480X16    3


/**
 * File stat
 */
//derived from stat.h, modified for use with DEX -- returned by fstat to hold info about the file
typedef struct _vfs_stat
{
        int     size;       /*The size of this structure*/
        int         st_dev;             /* Equivalent to drive number 0=A 1=B ... */
        int         st_ino;             /* Always zero ? */
        int         st_mode;    /* See above constants */
        short       st_nlink;       /* Number of links. */
        short       st_uid;         /* User: Maybe significant on NT ? */
        short       st_gid;         /* Group: Ditto */
        int         st_rdev;    /* Seems useless (not even filled in) */
        int         st_size;    /* File size in bytes */
        int         st_atime;   /* Accessed date (always 00:00 hrs local
                                 * on FAT) */
        int         st_mtime;   /* Modified time */
        int         st_ctime;   /* Creation time */
} vfs_stat;





/*POSIX typedefs*/
typedef unsigned int mode_t,dev_t,gid_t,ino_t,nlink_t,off_t,uid_t,clock_t,size_t;
typedef long int time_t;
typedef int pid_t,ssize_t;
typedef int (*fnptr_t)(unsigned c, void **helper,FILE *f);
typedef int (*sfnptr_t)(unsigned c, void **helper);
typedef void (*sighandler_t)(int signum);


/*********stdarg types from DJGPP*************/
typedef void *va_list;

#define __dj_va_rounded_size(T)  \
  (((sizeof (T) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

#define va_arg(ap, T) \
    (ap = (va_list) ((char *) (ap) + __dj_va_rounded_size (T)),	\
     *((T *) (void *) ((char *) (ap) - __dj_va_rounded_size (T))))

#define va_end(ap)

#define va_start(ap, last_arg) ((void)((ap) = \
     (va_list)((char *)(&last_arg)+__dj_va_rounded_size(last_arg))))  
     
#define unconst(__v, __t) __extension__ ({union { const __t __cp; __t __p; } __q; __q.__cp = __v; __q.__p;})
     
void  clrscr();
int do_printf(const char *fmt, va_list args, fnptr_t fn,FILE *f, void *ptr); 
void exit (int status);
void free(void *ptr);
int kb_deq(int *code);
int getchar();
char getch();
void getparameters(char *buf);
int getx();
int gety();
void gotoxy(int x,int y);
int printf(const char *fmt, ...);
unsigned int dexsdk_systemcall(int function_number,int p1,int p2,
                  int p3,int p4,int p5);
void *malloc(size_t size);
void *memmove (void *dst, const void *src,unsigned int count);
void * memset (void *dst,int val,unsigned int count);
void *memchr(const void *s, int c, size_t n);
void * memcpy (void * dst, const void * src,unsigned int count);
int memcmp(const void *s1, const void *s2, size_t n);
void *realloc(void *ptr, size_t size);
char *strcpy(char *to, const char *from);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
int strcmp(const char *s1, const char *s2);
size_t strcspn(const char *s1, const char *s2);
int strcoll(const char *s1, const char *s2);
size_t strspn(const char *s1, const char *s2);
char *strcat(char *s, const char *append);
char *strstr(const char *s, const char *find);
char *strncat(char *dst, const char *src, size_t n);
char *strncpy(char *dst, const char *src, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);
void *sbrk(int amt);
void setx(int x);
void sety(int y);
size_t strlen(const char *str);
char *strtok(char *s, const char *delim);
void outc(char x);
void textbackground(char val);
void textcolor(char val);
void update_cursor(int x,int y);
int wherex(void);
int wherey(void);

FILE *openfile(const char  *filename,int mode);
int feof(FILE *f);
FILE *fopen(const char *filename,const char *s);
int fgetc (FILE *stream);
char *fgets(char *s, int n, FILE* f);
int fread(const void *buf,int itemsize,int noitems,FILE* fhandle);
int fwrite(const void *buf,int itemsize,int noitems,FILE* fhandle);
char fputc(char c,FILE *f);
int fclose(FILE *stream);
int fflush (FILE *stream);
char *fseek(FILE* f,long x,int y);
long int ftell(FILE *stream);
int closefile(FILE* fhandle);
int remove(char *filename);
int mkdir (const char *filename, mode_t mode);
int copyfile(const char *src, const char *dest);

//VGA functions added by jach from dex api
void setgraphics(int mode);
void writepixel(int x, int y, char color);
void read_palette(char *r, char *g, char *b, char index);
void write_palette(char r, char g, char b, char index);

#endif 
