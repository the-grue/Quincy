/* ------ stdio.h ------- */

#ifndef _STDIO_H
#define _STDIO_H

#ifndef NULL
#define NULL (void*)0
#endif
#define EOF -1
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef int FILE;

#define stdin  (FILE*)0
#define stdout (FILE*)1
#define stderr (FILE*)2
#define stdaux (FILE*)3
#define stdprn (FILE*)4

/* ----- Console I/O prototypes ----- */
int getchar(void);
int putchar(int);
char *gets(char*);
int puts(const char*);
/* --- File I/O prototypes --- */
FILE *fopen(const char*,const char*);
int fclose(FILE*);
int fgetc(FILE*);
#define getc(f) fgetc(f)
int ungetc(int,FILE*);
int fputc(int,FILE*);
#define putc(c,f) fputc(c,f)
char *fgets(void*,int,FILE*);
int fputs(const char*,FILE*);
int fread(void*,int,int,FILE*);
int fwrite(const void*,int,int,FILE*);
long ftell(FILE*);
int fseek(FILE*,long,int);
int rename(const char*,const char*);
void rewind(FILE*);
int fflush(FILE*);
int remove(const char*);
FILE *tmpfile(void);
char *tmpnam(char*);

/* ----- Format conversion prototypes ----- */
int printf(const char*,...);
int scanf(const char*,...);
int fprintf(FILE*,const char*,...);
int fscanf(FILE*,const char*,...);
int sprintf(char*,const char*,...);
int sscanf(char*,const char*,...);

#endif

