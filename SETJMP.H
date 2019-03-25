/* ----- setjmp.h ----- */

#ifndef _SETJMP_H
#define _SETJMP_H

typedef struct _jb {
    char a[105];
} jmp_buf[1];

int setjmp(jmp_buf);
void longjmp(jmp_buf,int);

#endif

