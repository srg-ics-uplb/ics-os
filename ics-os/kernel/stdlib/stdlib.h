/*
  Name: DEX32 Standard C library function collection
  Copyright: GPL
  Author: Code from the GNU C libc sources by DJ Delorie
  Date: 23/10/03 17:23
  Description: Provides standard C functions to be used by the OS modules
*/
#define ERANGE		2

int errno;

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
long strtol(const char *nptr, char **endptr, int base);
int strsort(const char *s1, const char *s2);
int atoi(const char *str);


