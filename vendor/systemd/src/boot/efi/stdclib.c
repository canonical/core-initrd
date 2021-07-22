/* Taken from grub. GPL3, consider taking from elsewhere?
   (https://clc-wiki.net/wiki) */
#include <stdint.h>
#include <stddef.h>

#include "stdclib.h"

void *
memmove (void *dest, const void *src, size_t n)
{
  char *d = (char *) dest;
  const char *s = (const char *) src;

  if (d < s)
    while (n--)
      *d++ = *s++;
  else
    {
      d += n;
      s += n;

      while (n--)
	*--d = *--s;
    }

  return dest;
}

int
memcmp (const void *s1, const void *s2, size_t n)
{
  const uint8_t *t1 = s1;
  const uint8_t *t2 = s2;

  while (n--)
    {
      if (*t1 != *t2)
	return (int) *t1 - (int) *t2;

      t1++;
      t2++;
    }

  return 0;
}

void *memchr(const void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char*)s;
    while (n--)
        if (*p != (unsigned char)c)
            p++;
        else
            return p;
    return 0;
}

size_t strlen (const char *s)
{
  const char *p = s;

  while (*p)
    p++;

  return p - s;
}

size_t strnlen(const char *s, size_t maxlen)
{
    size_t i = 0;
    for(; (i < maxlen) && s[i]; ++i);
    return i;
}

char *strchr(const char *s, int c)
{
  do
    {
      if (*s == c)
	return (char *) s;
    }
  while (*s++);

  return 0;
}

char *strrchr(const char *s, int c)
{
  char *p = 0;

  do
    {
      if (*s == c)
	p = (char *) s;
    }
  while (*s++);

  return p;
}

int isspace (int c)
{
  return (c == '\n' || c == '\r' || c == ' ' || c == '\t');
}

static inline int
tolower(int c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';

  return c;
}

/* Divide N by D, return the quotient, and store the remainder in *R.  */
uint64_t
grub_divmod64 (uint64_t n, uint64_t d, uint64_t *r)
{
  /* This algorithm is typically implemented by hardware. The idea
     is to get the highest bit in N, 64 times, by keeping
     upper(N * 2^i) = (Q * D + M), where upper
     represents the high 64 bits in 128-bits space.  */
  unsigned bits = 64;
  uint64_t q = 0;
  uint64_t m = 0;

  while (bits--)
    {
      m <<= 1;

      if (n & (1ULL << 63))
	m |= 1;

      q <<= 1;
      n <<= 1;

      if (m >= d)
	{
	  q |= 1;
	  m -= d;
	}
    }

  if (r)
    *r = m;

  return q;
}

unsigned long long
strtoull (const char *str, char **end, int base)
{
  unsigned long long num = 0;
  int found = 0;

  /* Skip white spaces.  */
  /* grub_isspace checks that *str != '\0'.  */
  while (isspace (*str))
    str++;

  /* Guess the base, if not specified. The prefix `0x' means 16, and
     the prefix `0' means 8.  */
  if (str[0] == '0')
    {
      if (str[1] == 'x')
	{
	  if (base == 0 || base == 16)
	    {
	      base = 16;
	      str += 2;
	    }
	}
      else if (base == 0 && str[1] >= '0' && str[1] <= '7')
	base = 8;
    }

  if (base == 0)
    base = 10;

  while (*str)
    {
      unsigned long digit;

      digit = tolower (*str) - '0';
      if (digit >= 'a' - '0')
	digit += '0' - 'a' + 10;
      else if (digit > 9)
	break;

      if (digit >= (unsigned long) base)
	break;

      found = 1;

      /* NUM * BASE + DIGIT > ~0ULL */
      if (num > grub_divmod64 (~0ULL - digit, base, 0))
	{
          if (end)
            *end = (char *) str;

	  return ~0ULL;
	}

      num = num * base + digit;
      str++;
    }

  if (! found)
    {
      if (end)
        *end = (char *) str;

      return 0;
    }

  if (end)
    *end = (char *) str;

  return num;
}

unsigned long strtoul(const char *str, char **end, int base)
{
  unsigned long long num;

  num = strtoull (str, end, base);

  return (unsigned long) num;
}
