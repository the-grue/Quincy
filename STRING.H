/* ------- string.h -------- */

#ifndef _STRING_H
#define _STRING_H

int strcmp(const char*,const char*);
int strncmp(const char*,const char*,int);
char *strcpy(char*,const char*);
char *strncpy(char*,const char*,int);
int strlen(const char*);
char *strcat(char*,const char*);
char *strncat(char*,const char*,int);

void *memset(void *s,int c,int n)
{
	char *cp = s;
	while (n--)
		*cp++ = c;
	return s;
}
#endif

