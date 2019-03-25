/* -------- stdlib.h ------- */

#ifndef _STDLIB_H
#define _STDLIB_H

#ifndef _STRING_H
#include <string.h>
#endif

int abs(int);
void *malloc(int);
void free(void*);
void exit(int);
int system(const char*);
float atof(const char*);
int atoi(const char*);
long atol(const char*);

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

void abort(void)
{
	exit(-1);
}

void *calloc(int sz, int n)
{
	int siz = sz*n;
	void *ptr;
	ptr = malloc(siz);
	if (ptr)
		memset(ptr, 0, siz);
	return ptr;
}

static unsigned long int _next = 1;

int rand(void)
{
	_next = _next * 1103515245 + 12345;
	return (unsigned) (_next/65536) % 32768;
}

void srand(unsigned int seed)
{
	_next = seed;
}

#endif

