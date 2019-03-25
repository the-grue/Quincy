/*
 * QUINCY Interpreter - sys calls
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <dos.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <dir.h>
#include <time.h>
#include "qnc.h"
#include "sys.h"
#include "debugger.h"

static int WasFileFunction, WasConsole;

extern int DupStdin;
extern int DupStdout;
void CloseStdout(void);

#define MAXALLOC 100
static int memctr = 0;
static char *allocs[MAXALLOC];
static void unstkmem(char *);

static FILE *OpenFiles[MAXOPENFILES];
static int OpenFileCount;

static FILE *AddOpenFile(FILE *fp)
{
	if (OpenFileCount == MAXOPENFILES)	{
		fclose(fp);
		return NULL;
	}
	OpenFiles[OpenFileCount++] = fp;
	return fp;
}

static void RemoveOpenFile(FILE *fp)
{
	int i;
	for (i = 0; i < OpenFileCount; i++)
		if (OpenFiles[i] == fp)
			break;
	for (--OpenFileCount;i < OpenFileCount; i++)
		OpenFiles[i] = OpenFiles[i+1];
}

void TerminateProgram(void)
{
	int i;
	for (i = 0; i < OpenFileCount; i++)
		fclose(OpenFiles[i]);
	OpenFileCount = 0;
}

static FILE *chkchannel(FILE *channel)
{
	/*
	 * Converts Quincy std handles to real ones
	 * Opens screen display in IDE for stdout, stderr
	 */
	static FILE *handles[] = {stdin,stdout,stderr,stdaux,stdprn};
	long lchan = (long) channel;
	WasFileFunction = 1;
	if (lchan < 5)	{
		switch (lchan)	{
			case 0:
				/* --- stdin --- */
				WasConsole = DupStdin == -1;
				break;
			case 1:
				/* --- stdout --- */
				WasConsole = DupStdout == -1;
				break;
			case 2:
				/* --- stderr --- */
				WasConsole = 1;
				break;
			default:
				WasConsole = 0;
				break;
		}
		if (WasConsole)
			OpenStdout();
		return handles[(int) lchan];
	}
	return channel;
}

struct vstack { char x[1000]; };
/* ---- convert scanf strings so that all %f or %Lf formats become %lf --- */
static char *cvtfmt(char *f2)
{
	char *f1, ch;
	static char ConvertedFormat[201];
	f1 = ConvertedFormat;
	while (*f2)	{
		Assert(f1 < ConvertedFormat+200);
		if ((*f1++ = *f2++) == '%')	{
			/* --- found a format specifier, bypass suppression char,
						width, && pointer size modifier --- */
			while (*f2 == '*' || isdigit(*f2) || *f2 == 'L')
				*f1++ = *f2++;
			ch = tolower(*f2);
			if ((*f2 == 'f' || ch == 'e' || ch == 'g') && *(f2-1) != 'l')	{
				if (*(f2-1) == 'L')	/* convert %Lf to %lf */
					*(f1-1) = 'l';
				else
					*f1++ = 'l';	/* convert %f to %lf */
			}
		}
	}
	*f1 = '\0';
	return ConvertedFormat;
}
/* ---- need these functions to get variable arguments
  on the stack, which is the only place that vprintf, etc,
  will process them when DS!=SS --- */
static int pprintf(char *fmt, struct vstack vs)
{
	return vprintf(fmt, &vs);
}
static int pscanf(char *fmt, struct vstack vs)
{
	return vscanf(cvtfmt(fmt), &vs);
}
static int psprintf(char *str, char *fmt, struct vstack vs)
{
	return vsprintf(str, fmt, &vs);
}
static int psscanf(char *str, char *fmt, struct vstack vs)
{
	return vsscanf(str, cvtfmt(fmt), &vs);
}
static int pfprintf(FILE *fp, char *fmt, struct vstack vs)
{
	FILE *fp1 = chkchannel(fp);
	return vfprintf(fp1, fmt, &vs);
}
static int pfscanf(FILE *fp, char *fmt, struct vstack vs)
{
	FILE *fp1 = chkchannel(fp);
	return vfscanf(fp1, cvtfmt(fmt), &vs);
}
static int pcprintf(char *fmt, struct vstack vs)
{
	char *buf = getmem(512);
	int rtn;
	vsprintf(buf, fmt, &vs);
	rtn = cprintf(buf);
	free(buf);
	return rtn;
}

static double (*mfunc[])(double) = {
	acos,asin,atan,ceil,cos,cosh,exp,fabs,floor,
	sin,sinh,sqrt,tan,tanh,log,log10
};

static void FixStacktmStructure(void)
{
	int symbolid = FindSymbol("tm");
	if (symbolid)	{
		VARIABLE *tvar = SearchVariable(symbolid, 1);
		if (tvar != NULL)
			Ctx.Stackptr->vstruct = tvar->vstruct;
	}
}

void sys(void)
{
	FILE *fp;
	char *cp, *cp1;
	int n, l;
	enum sysfuncs c;
	double f1, f2;
	int x, y, ch;

	/* first argument on stack is the sys function number */
	c = popint();

	WasFileFunction = WasConsole = 0;
	switch (c)	{
		/*
	 	 * System functions
	 	 */
		case SYSEXIT:
			longjmp(Shelljmp, 5);
		case SYSSYSTEM:
			OpenStdout();
			pushint(system(popptr()));
			if (Stepping)
				PromptIDE();
			CloseStdout();
			return;
		case SYSFINDFIRST:
			n = popint();
			cp1 = popptr();
			cp = popptr();
			pushint(findfirst(cp, (struct ffblk *)cp1, n));
			return;
		case SYSFINDNEXT:
			pushint(findnext(popptr()));
			return;
		/*
	 	 * String conversion functions
	 	 */
		case SYSATOF:
			pushflt(atof(popptr()));
			return;
		case SYSATOI:
			pushint(atoi(popptr()));
			return;
		case SYSATOL:
			pushlng(atol(popptr()));
			return;
		/*
	 	 * String functions
	 	 */
		case SYSSTRCMP:
			cp1 = popptr();
			cp = popptr();
			pushint(strcmp(cp, cp1));
			return;
		case SYSSTRNCMP:
			n = popint();
			cp1 = popptr();
			cp = popptr();
			pushint(strncmp(cp, cp1,n));
			return;
		case SYSSTRCPY:
			cp1 = popptr();
			cp = popptr();
			pushptr(strcpy(cp, cp1), CHAR);
			return;
		case SYSSTRNCPY:
			n = popint();
			cp1 = popptr();
			cp = popptr();
			pushptr(strncpy(cp, cp1,n), CHAR);
			return;
		case SYSSTRLEN:
			pushint(strlen(popptr()));
			return;
		case SYSSTRCAT:
			cp1 = popptr();
			cp = popptr();
			pushptr(strcat(cp, cp1), CHAR);
			return;
		case SYSSTRNCAT:
			n = popint();
			cp1 = popptr();
			cp = popptr();
			pushptr(strncat(cp, cp1,n), CHAR);
			return;
		/*
		 * memory allocation functions
	 	 */
		case SYSMALLOC:
		{
			int siz = popint();
			char *mem = malloc(siz);
			if (mem != NULL && memctr < MAXALLOC)
				allocs[memctr++] = mem;
			else	{
				free(mem);
				mem = NULL;
			}
			pushptr(mem, VOID);
			return;
		}
		case SYSFREE:
			unstkmem(popptr());
			pushint(0);
			return;
		/*
	 	 * Format conversion functions
	 	 */
		case SYSSSCANF:
			pushint(psscanf(
				*(char**)Ctx.Curfunc->ldata,
				*(char**)(Ctx.Curfunc->ldata+sizeof(char*)),
				*(struct vstack *)(Ctx.Curfunc->ldata+(sizeof(char*)*2))));
			return;
		case SYSSPRINTF:
			pushint(psprintf(
				*(char**)Ctx.Curfunc->ldata,
				*(char**)(Ctx.Curfunc->ldata+sizeof(char*)),
				*(struct vstack *)(Ctx.Curfunc->ldata+(sizeof(char*)*2))));
			return;

		case SYSABS:
			pushint(abs(popint()));
			return;
		/*
	 	 * File I/O functions
	 	 */
		case SYSFSEEK:
		{
			int md = popint();
			long j = poplng();
			fp = chkchannel(popptr());
			pushint(fseek(fp,j,md));
			return;
		}
		case SYSFOPEN:
		{
			char *md = popptr();
			char *fn = popptr();
			if ((fp = fopen(fn, md)) != NULL)
				fp = AddOpenFile(fp);
			pushptr(fp, INT);
			return;
		}
		case SYSTMPFILE:
			if ((fp = tmpfile()) != NULL)
				fp = AddOpenFile(fp);
			pushptr(fp, INT);
			return;
		case SYSTMPNAM:
			pushptr(tmpnam(popptr()), CHAR);
			return;
		case SYSREMOVE:
			pushint(remove(popptr()));
			return;
		case SYSFCLOSE:
			fp = popptr();
			RemoveOpenFile(fp);
			pushint(fclose(fp));
			return;
		case SYSFFLUSH:
			fp = chkchannel(popptr());
			pushint(fflush(fp));
			break;
		case SYSFGETC:
			fp = chkchannel(popptr());
			pushint(getc(fp));
			break;
		case SYSUNGETC:
			fp = chkchannel(popptr());
			pushint(ungetc(popint(), fp));
			break;
		case SYSFPUTC:
			fp = chkchannel(popptr());
			pushint(putc(popint(), fp));
			break;
		case SYSFGETS:
			fp = chkchannel(popptr());
			n = popint();
			pushptr(fgets(popptr(), n, fp), CHAR);
			break;
		case SYSFPUTS:
			fp = chkchannel(popptr());
			pushint(fputs(popptr(), fp));
			break;
		case SYSFREAD:
			fp = chkchannel(popptr());
			n = popint();
			l = popint();
			cp = popptr();
			pushint(fread(cp ,l ,n ,fp));
			break;
		case SYSFWRITE:
			fp = chkchannel(popptr());
			n = popint();
			l = popint();
			cp = popptr();
			pushint(fwrite(cp ,l ,n ,fp));
			break;
		case SYSFTELL:
			fp = chkchannel(popptr());
			pushlng(ftell(fp));
			break;
		case SYSRENAME:
			cp1 = popptr();
			cp = popptr();
			pushint(rename(cp, cp1));
			return;
		case SYSREWIND:
			fp = chkchannel(popptr());
			rewind(fp);
			pushint(0);
			break;
		case SYSFSCANF:
			pushint(pfscanf(
				*(FILE**)Ctx.Curfunc->ldata,
				*(char**)(Ctx.Curfunc->ldata+sizeof(char*)),
				*(struct vstack *)(Ctx.Curfunc->ldata+sizeof(FILE*)+sizeof(char*))));
			break;
		case SYSFPRINTF:
			pushint(pfprintf(
				*(FILE**)Ctx.Curfunc->ldata,
				*(char**)(Ctx.Curfunc->ldata+sizeof(char*)),
				*(struct vstack *)(Ctx.Curfunc->ldata+sizeof(FILE*)+sizeof(char*))));
			break;
		case SYSASCTIME:
			pushptr(asctime(popptr()), CHAR);
			return;
		case SYSGMTIME:
			pushptr(gmtime(popptr()), STRUCT);
			Ctx.Stackptr->size = sizeof(struct tm);
			FixStacktmStructure();
			return;
		case SYSLOCALTIME:
			pushptr(localtime(popptr()), STRUCT);
			Ctx.Stackptr->size = sizeof(struct tm);
			FixStacktmStructure();
			return;
		case SYSMKTIME:
			pushlng(mktime(popptr()));
			return;
		case SYSTIME:
			pushlng(time(popptr()));
			return;
		default:
			if (c >= SYSACOS)	{
				/*
		 		* math function
		 		*/
				f1 = popflt();
				if (c == SYSPOW)	{
					f2 = popflt();
					pushflt(pow(f1, f2));
				}
				else if (c == SYSATAN2)	{
					f2 = popflt();
					pushflt(atan2(f1, f2));
				}
				else if (c < SYSPOW)
					pushflt((*mfunc[ c - SYSACOS ]) (f1));
				else
					pushint(0);
				return;
			}
			break;
	}

	if (WasFileFunction)	{
		if (WasConsole)
			CloseStdout();
		return;
	}

	/* --- the remaining functions will or might use the screen --- */
	switch (c)	{
		/*
	 	 * Console input functions
 	 	 */
		case SYSGETCH:
			OpenStdout();
			if ((ch = getch()) == 3)
				CBreak();
			if (ch == 0)
				ch = getch() | 0x80;
			pushint(ch);
			CloseStdout();
			return;
		case SYSPUTCH:
			OpenStdout();
			putch(popint());
			pushint(0);
			CloseStdout();
			return;
		case SYSCLRSCRN:
			OpenStdout();
			clearscreen();
			pushint(0);
			CloseStdout();
			return;
		case SYSCURSOR:
			OpenStdout();
			y = popint();
			x = popint();
			cursor(x, y);
			pushint(0);
			CloseStdout();
			return;
		case SYSGETCHAR:
			if (DupStdin == -1)
				OpenStdout();
			pushint(getchar());
			if (DupStdin == -1)
				CloseStdout();
			return;
		case SYSGETS:
			if (DupStdin == -1)
				OpenStdout();
			pushptr(gets(popptr()), CHAR);
			if (DupStdin == -1)
				CloseStdout();
			return;
		case SYSSCANF:
			if (DupStdin == -1)
				OpenStdout();
			pushint(pscanf(*(char**)Ctx.Curfunc->ldata,
				*(struct vstack *)(Ctx.Curfunc->ldata+sizeof(char*))));
			if (DupStdin == -1)	{
				fflush(stdin);
				CloseStdout();
			}
			return;
		default:
			break;
	}

	switch (c)	{
		/*
	 	 * Console output functions
 	 	 */
		case SYSPUTCHAR:
			if (DupStdout == -1)
				OpenStdout();
			pushint(putchar(popint()));
			if (DupStdout == -1)
				CloseStdout();
			break;
		case SYSPUTS:
			if (DupStdout == -1)
				OpenStdout();
			pushint(puts(popptr()));
			if (DupStdout == -1)
				CloseStdout();
			break;
		case SYSPRINTF:
			if (DupStdout == -1)
				OpenStdout();
			pushint(pprintf(*(char**)Ctx.Curfunc->ldata,
				*(struct vstack *)(Ctx.Curfunc->ldata+sizeof(char*))));
			if (DupStdout == -1)
				CloseStdout();
			break;
		case SYSCPRINTF:
			OpenStdout();
			pushint(pcprintf(*(char**)Ctx.Curfunc->ldata,
				*(struct vstack *)(Ctx.Curfunc->ldata+sizeof(char*))));
			CloseStdout();
			break;
		default:
			break;
	}
}

/* -------- keep track of mallocs --------- */

/* --------- remove all unallocated blocks --------- */
void ClearHeap(void)
{
	int i;

	for (i = 0; i < memctr; i++)
		free(allocs [i]);
	memctr = 0;
}

/* -------- remove an allocated block from the list --------- */
static void unstkmem(char *adr)
{
	int i;

	free(adr);
	for (i = 0; i < memctr; i++)
		if (adr == allocs[i])
			break;
	if ( i < memctr )	{
		--memctr;
		while ( i < memctr )	{
			allocs[i] = allocs[i+1];
			i++;
		}
	}
}

