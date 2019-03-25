/*
 * QUINCY Runtime Interpreter
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <setjmp.h>
#include <sys\stat.h>
#include <alloc.h>
#include <errno.h>

#include "dflat.h"
#include "qnc.h"
#include "debugger.h"
#include "interp.h"

CONTEXT Ctx;
unsigned char *Progstart;	/* start of user program */
unsigned char *NextProto;	/* addr of next prototype */
unsigned Progused;			/* bytes of program space used */
int Saw_return;		/* set when "return" encountered in user program */
int Saw_break;		/* set when "break" encountered in user program */
int Saw_continue;	/* set when "continue" encountered in user program */
int Linking;		/* set when in linker */

unsigned char *pSrc;
ITEM *Stackbtm;			  /* start of program stack */
ITEM *Stacktop;			  /* end of program stack */
SYMBOLTABLE *SymbolTable; /* symbol table */
int SymbolCount;
FUNCTION *NextFunction;	  /* next available function in table */
VARIABLE *Blkvar;		  /* beginning of local block variables */
VARIABLELIST Globals;	  /* table of program global variables */

VARIABLE *VariableMemory;
FUNCTION *FunctionMemory;
char *DataSpace;
char *PrototypeMemory;

jmp_buf Shelljmp;

/*
 * table size variables
 */
struct SREGS segs;
union REGS rgs;

static int ExecuteProgram(unsigned char *source, int argc, char *argv[]);
static void qprload(unsigned char *SourceCode, char *prog);

static void ClearMemory(void **buf, void **end, int *count)
{
	free(*buf);
	*buf = NULL;
	if (end)
		*end = NULL;
	if (count)
		*count = 0;
}

int qinterpmain(unsigned char *source, int argc, char *argv[])
{
	int rtn = -1;

	Globals.vfirst = NULL;
	Globals.vlast = NULL;
	Ctx.Curvar = NULL;
	Ctx.Curstruct.vfirst = NULL;
	Ctx.Curstruct.vlast = NULL;
	Ctx.Curfunc = NULL;
	ConstExpression = 0;

	errno = 0;
	Progstart = getmem(qCfg.MaxProgram);
	/*
	 * Initialize program variables
	 */
	Ctx.Progptr = Progstart;
	/*
	 * Allocate memory for stack, variables, data, and functions
	 */
	Stackbtm = getmem((qCfg.MaxStack+1) * sizeof(struct item));
	Ctx.Stackptr = Stackbtm;
	Stacktop = Stackbtm + qCfg.MaxStack;

	VariableMemory = getmem(qCfg.MaxVariables * sizeof(VARIABLE));
	Ctx.NextVar = VariableMemory;

	if ((DataSpace = malloc(qCfg.MaxDataSpace)) == NULL)
		error(OMERR);
	Ctx.NextData = DataSpace;

	FunctionMemory = getmem(qCfg.MaxFunctions * sizeof(FUNCTION));
	NextFunction = FunctionMemory;

	SymbolTable = getmem(qCfg.MaxSymbolTable * sizeof(SYMBOLTABLE));

	NextProto = PrototypeMemory = getmem(qCfg.MaxPrototype);
	*ErrorMsg = '\0';

	fflush(stdin);
	fflush(stdout);

	rtn = ExecuteProgram(source, argc, argv);

	ClearHeap();
	DeleteSymbols();
	if (ErrorCode && Ctx.CurrFileno)
		sprintf(ErrorMsg+strlen(ErrorMsg), " %s Line %d: ", SrcFileName(Ctx.CurrFileno), Ctx.CurrLineno);
	CleanUpPreProcessor();
	ClearMemory(&(void*)pSrc, NULL, NULL);
	ClearMemory(&(void*)PrototypeMemory, &(void*)NextProto, NULL);
	ClearMemory(&(void*)SymbolTable, NULL, &SymbolCount);
	ClearMemory(&(void*)FunctionMemory, &(void*)NextFunction, NULL);
    ClearMemory(&(void*)DataSpace, &(void*)Ctx.NextData, NULL);
	ClearMemory(&(void*)VariableMemory, &(void*)Ctx.NextVar, NULL);
	ClearMemory(&(void*)Stackbtm, &(void*)Ctx.Stackptr, NULL);
	ClearMemory(&(void*)Progstart, NULL, &(int)Progused);
	errno = 0;
	return rtn;
}

static int ExecuteProgram(unsigned char *source, int argc, char *argv[])
{
	unsigned char Tknbuf[80];
	unsigned char ln[40];
    WINDOW wwnd = WatchIcon();
	if (setjmp(Shelljmp) == 0)	{
		qprload(source, Progstart);
		link(&Globals);

		sprintf(ln, "return main(%d,(char**)%lu);", argc, argv);
		tokenize(Tknbuf, ln);
		Ctx.Progptr = Tknbuf;
		getoken();
	    SendMessage(wwnd, CLOSE_WINDOW, 0, 0);
		wwnd = NULL;
		if (!Stepping)
			HideIDE();
		statement();
	}
	if (wwnd != NULL)
	    SendMessage(wwnd, CLOSE_WINDOW, 0, 0);
	TerminateProgram();
	return ErrorCode ? ErrorCode : popint(); 
}


static void qprload(unsigned char *SourceCode, char *prog)
{
	/*
	 * tokenize program
	 */
	pSrc = getmem(MAXTEXTLEN);
	PreProcessor(pSrc, SourceCode);
	Progused = tokenize(prog, pSrc);
	free(pSrc);
	pSrc = NULL;
	Ctx.Progptr = Progstart;
}

void error(int errnum)
{
	ErrorCode = errnum;
	if (Watching)
		longjmp(Watchjmp, 1);
	else if (Running)
		longjmp(Shelljmp, 1);
}

void *getmem(unsigned size)
{
	void *ptr;
	if ((ptr = calloc(1, size)) == NULL)
		error(OMERR);
	return ptr;
}

#ifndef NDEBUG
void AssertFail(char *cond, char *file, int lno)
{
	sprintf(errs[ASSERTERR-1], "Assert(%s) %s, Line %d", cond, file, lno);
	error(ASSERTERR);
}
#endif




