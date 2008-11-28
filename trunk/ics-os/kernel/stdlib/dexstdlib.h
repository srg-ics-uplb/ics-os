/*
  Name: DEX32 Standard C library function collection
  Copyright: GPL
  Author: Code from the GNU C libc sources by DJ Delorie
  Date: 23/10/03 17:23
  Description: Provides standard C functions to be used by the OS modules
*/
#define unconst(__v, __t) __extension__ ({union { const __t __cp; __t __p; } __q; __q.__cp = __v; __q.__p;})

typedef unsigned int size_t;

void *memchr(const void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
char *strcat(char *s, const char *append);
char *strchr(const char *s, int c);
int strcmp(const char *s1, const char *s2);
int strcoll(const char *s1, const char *s2);
char *strcpy(char *to, const char *from);
size_t strcspn(const char *s1, const char *s2);
size_t strlen(const char *str);
char *strncat(char *dst, const char *src, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *dst, const char *src, size_t n);
char *strpbrk(const char *s1, const char *s2);
char *strrchr(const char *s, int c);
size_t strspn(const char *s1, const char *s2);
char *strstr(const char *s, const char *find);
char *strtok(char *s, const char *delim);
void *memset (void *dst,int val,unsigned int count);
char tolower(char c);
char toupper(char c);
void * memcpy (void * dst, const void * src,unsigned int count);
void *memmove (void *dst, const void *src,unsigned int count);
char *strupr(char *str);
char *itoa (int val,char *buf,int radix);
       
