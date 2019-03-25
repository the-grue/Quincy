/* ----- assert.h ----- */

#ifndef _ASSERT_H
#define _ASSERT_H

#ifndef _STDIO_H
#include <stdio.h>
#endif

#ifndef _STDLIB_H
#include <stdlib.h>
#endif

#ifdef NDEBUG
#define assert(n) ((void)0)
#else

int _lineno(void);
char *_filename(void);

void _assertfail(char *s)
{
	char *fn = _filename();
	printf("\n\aassert failed: (%s) File: %s, Line: %d\n", 
				s,_filename(), _lineno());
	abort();
}

#define assert(n) (n)?((void)0):(_assertfail(#n))

#endif

#endif

