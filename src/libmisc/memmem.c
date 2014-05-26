/*
**  MEMMEM.C - A strstr() work-alike for non-text buffers
**
**  public domain by Bob Stout
**
**  Warning: The memchr() in Borland C/C++ versions *prior* to 4.x is broken!
** 
**  Fixed by Thomas Sailer
*/

#include <string.h>

void *memmem(const void *buf, size_t buflen, const void *pattern, size_t len)
{
	char *bf = (char *)buf, *pt = (char *)pattern, *p = bf;

	while (len <= (buflen - (p - bf))) {
		if (NULL != (p = memchr(p, (int)(*pt), buflen - (p - bf) - len + 1))) {
			if (!memcmp(p, pattern, len))
				return p;
			else  ++p;
		} else
			break;
	}
	return NULL;
}
