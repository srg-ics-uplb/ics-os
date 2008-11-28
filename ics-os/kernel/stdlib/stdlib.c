
int	isspace(int ci)
{
    char c=(char)ci;
    if (c==' ') return 1;
    return 0;
};

int	isdigit(int ci)
{
    char c=(char)ci;
    if ('0' <= c && c <= '9') return 1;
    return 0;
};

int	isalpha(int ci)
{
    char c=(char)ci;
    if ( (c>='a'&&c<='z') ||
         (c>='A'&&c<='Z')) return 1;
    return 0;     
};

int	isupper(int ci)
{
    char c=(char)ci;
    if (c>='A'&&c<='Z') return 1;
    return 0;
};

long
strtol(const char *nptr, char **endptr, int base)
{
  const char *s = nptr;
  unsigned long acc;
  int c;
  unsigned long cutoff;
  int neg = 0, any, cutlim;

  /*
   * Skip white space and pick up leading +/- sign if any.
   * If base is 0, allow 0x for hex and 0 for octal, else
   * assume decimal; if base is already 16, allow 0x.
   */
  do {
    c = *s++;
  } while (isspace(c & 0xff));
  if (c == '-')
  {
    neg = 1;
    c = *s++;
  }
  else if (c == '+')
    c = *s++;
  if ((base == 0 || base == 16) &&
      c == '0' && (*s == 'x' || *s == 'X'))
  {
    c = s[1];
    s += 2;
    base = 16;
  }
  if (base == 0)
    base = c == '0' ? 8 : 10;

  /*
   * Compute the cutoff value between legal numbers and illegal
   * numbers.  That is the largest legal value, divided by the
   * base.  An input number that is greater than this value, if
   * followed by a legal input character, is too big.  One that
   * is equal to this value may be valid or not; the limit
   * between valid and invalid numbers is then based on the last
   * digit.  For instance, if the range for longs is
   * [-2147483648..2147483647] and the input base is 10,
   * cutoff will be set to 214748364 and cutlim to either
   * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
   * a value > 214748364, or equal but the next digit is > 7 (or 8),
   * the number is too big, and we will return a range error.
   *
   * Set any if any `digits' consumed; make it negative to indicate
   * overflow.
   */
  cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
  cutlim = cutoff % (unsigned long)base;
  cutoff /= (unsigned long)base;
  for (acc = 0, any = 0;; c = *s++, c &= 0xff)
  {
    if (isdigit(c))
      c -= '0';
    else if (isalpha(c))
      c -= isupper(c) ? 'A' - 10 : 'a' - 10;
    else
      break;
    if (c >= base)
      break;
    if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
      any = -1;
    else
    {
      any = 1;
      acc *= base;
      acc += c;
    }
  }
  if (any < 0)
  {
    acc = neg ? LONG_MIN : LONG_MAX;
    errno = ERANGE;
  }
  else if (neg)
    acc = -acc;
  if (endptr != 0)
    *endptr = any ? unconst(s, char *) - 1 : unconst(nptr, char *);
  return acc;
}



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
};

int strsort(const char *s1, const char *s2)
{
    int i;
    for ( i = 0; s1[i] && s2[i]; i++)
    {
      if (s1[i]>s2[i]) return 1;
      if (s1[i]<s2[i]) return -1;
    };
    
    if (s1[i]) return 1;
    if (s2[i]) return -1;
    
    return 0;
};

char *strcat(char *s, const char *append)
{
  char *save = s;

  for (; *s; ++s);
  while ((*s++ = *append++));
  return save;
}

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
};

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
};

int strcoll(const char *s1, const char *s2)
{
  return strcmp(s1, s2);
};

char *strcpy(char *to, const char *from)
{
  char *save = to;

  for (; (*to = *from); ++from, ++to);
  return save;
};

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
};

size_t strlen(const char *str)
{
  const char *s;

  if (str == 0)
    return 0;
  for (s = str; *s; ++s);
  return s-str;
};

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
};


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
};


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
};

char *strpbrk(const char *s1, const char *s2)
{
  const char *scanp;
  int c, sc;

  while ((c = *s1++) != 0)
  {
    for (scanp = s2; (sc = *scanp++) != 0;)
      if (sc == c)
	return unconst(s1 - 1, char *);
  }
  return 0;
};

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
};


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
};

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
};

//strtok has been fixed so that it will work in a 
//multithreaded environment
char *strtok(char *s, const char *delim)
{
  const char *spanp;
  int c, sc;
  char *tok;
  char **last=&current_process->misc;


  if (s == NULL && (s = *last) == NULL)
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
    *last = NULL;
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
	*last = s;
	return (tok);
      }
    } while (sc != 0);
  }
  /* NOTREACHED */
};

void * memset (
        void *dst,
        int val,
        unsigned int count
        )
{
        void *start = dst;

        while (count--) {
                *(char *)dst = (char)val;
                dst = (char *)dst + 1;
        }

        return(start);
};

char tolower(char c)
 {
  if (c>='A'&&c<='Z') return (c-'A'+'a');
  return c;
 };

char toupper(char c)
 {
  if (c>='a'&&c<='z') return (c-'a'+'A');
  return c;
 };
 
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

char * ttyname (int filedes)
{
 return 0;
};

//moves one memory location to another
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

char *strupr(char *str)
 {
 char *s=str;
   while (*str)
    {
     *str=toupper(*str);
     str++;
    };
   return s;
 };

extern char *itoa (
        int val,
        char *buf,
        int radix
        );

int atoi(const char *str)
{
    int i = strlen(str) - 1 , i2 = 1;
    int num = 0;
    for (;i>=0; i--)
      {
        if ( (str[i]>='0') && (str[i]<='9') )
        {
            num += (str[i]-'0') * i2;
            i2*=10;
        };
      };
    return num;  
};
                        
void xtoa (
        unsigned long val,
        char *buf,
        unsigned radix,
        int is_neg
        )
{
        char *p;                /* pointer to traverse string */
        char *firstdig;         /* pointer to first digit */
        char temp;              /* temp char */
        unsigned digval;        /* value of digit */

        p = buf;

        if (is_neg) {
            /* negative, so output '-' and negate */
            *p++ = '-';
            val = (unsigned long)(-(long)val);
        }

        firstdig = p;           /* save pointer to first digit */

        do {
            digval = (unsigned) (val % radix);
            val /= radix;       /* get next digit */

            /* convert to ascii and store */
            if (digval > 9)
                *p++ = (char) (digval - 10 + 'a');  /* a letter */
            else
                *p++ = (char) (digval + '0');       /* a digit */
        } while (val > 0);

        /* We now have the digit of the number in the buffer, but in reverse
           order.  Thus we reverse them now. */

        *p-- = '\0';            /* terminate string; p points to last digit */

        do {
            temp = *p;
            *p = *firstdig;
            *firstdig = temp;   /* swap *p and *firstdig */
            --p;
            ++firstdig;         /* advance to next two digits */
        } while (firstdig < p); /* repeat until halfway */
};


