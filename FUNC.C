/*
 * QUINCY Interpreter - function calling routines
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <errno.h>
#include "sys.h"
#include "qnc.h"

static void ArgumentList(int, ITEM *);
int longjumping;
int inSystem;
jmp_buf BreakJmp;

extern JMPBUF stmtjmp;

extern jmp_buf statement_jmpbuf;
static int jmp_val = 0;

void callfunc()
{
	/*
	 * Call a function
	 */
	unsigned char *svprogptr;
	FUNCTION *svCurfunction = Ctx.Curfunction;
	ITEM *args, *proargs;
	int argc, i;
	FUNCRUNNING func;
	char c;

	/*
	 * If there are any arguments, evaluate them and leave their values
	 * on the stack.
	 */
	proargs = args = Ctx.Stackptr + 1;
	getoken();		/* get rid of the '(' */
	argc = expression();
	if (Ctx.Token!=')')
		error(RPARENERR);

	Ctx.Curfunction = svCurfunction;
	svprogptr = Ctx.Progptr;

	/*
	 * It's a call on a C function.  Save current
	 * program pointer and set it to start address of
	 * called function.
	 */

	if (!Ctx.Curfunction->ismain)	{
		VARIABLE *svar;
		unsigned char *typ;
		int proargc = argc, indir = 0;

		/*
		 * Compare the parameters passed and the function's prototype
		 */
		typ = Ctx.Curfunction->proto;
		typ = typ ? typ : "";
		if (proargc == 0 && *typ != 0xff && *typ != VOID)
			error(MISMATCHERR);
		while (*typ != 0xff && *typ != T_ELLIPSE && proargc)	{
			switch (*typ)	{
				case CHAR:
				case INT:
				case LONG:
				case FLOAT:
					if (proargs->type > FLOAT)
						error(MISMATCHERR);
					break;
				case VOID:
					if (!*(typ+1))
						error(MISMATCHERR);
					break;
				case STRUCT:
				case UNION:
					if (proargs->type != *typ)
						error(MISMATCHERR);
					svar = *(VARIABLE**)(typ+1);
					Assert(svar != NULL);
					typ += sizeof(VARIABLE*);
					if (proargs->vstruct != svar->vstruct)
						error(MISMATCHERR);
					break;
			}
			indir = *++typ;
			if (proargs->class != indir)
				error(MISMATCHERR);
			--proargc;
			proargs++;
			typ++;
		}
		if (*typ != T_ELLIPSE && *typ != VOID && (proargc || *typ != 0xff))
			error(MISMATCHERR);
	}
	Ctx.Progptr = (unsigned char *)Ctx.Curfunction->code;
	func.fvar = Ctx.Curfunction;
	func.fprev = Ctx.Curfunc;
	func.BlkNesting = 0;
	func.ldata = Ctx.NextData;
	func.arglength = 0;
	Ctx.Curfunc = &func;

	Saw_return = 0;
	if (Ctx.Progptr == NULL)	{
		if (Ctx.Curfunction->libcode == 0)
			error(Ctx.Curfunction->ismain ? NOMAINERR : NOFUNCERR);
		switch (Ctx.Curfunction->libcode)	{
			case SYSERRNO:
				pushptr(&errno, INT);
				break;
			case SYSLINENO:
				pushint(Ctx.CurrLineno);
				break;
			case SYSFILENAME:
				pushptr(SrcFileName(Ctx.CurrFileno), CHAR);
				break;
			case SYSSETJMP:
				if (longjumping)	{
					pushint(jmp_val);
					jmp_val = 0;
					longjumping = FALSE;
				}
				else	{
					JMPBUF *thisjmp = popptr();
					*thisjmp = stmtjmp;
					thisjmp->jmp_id = 0x0a0a;
					pushint(0);
				}
				break;
			case SYSLONGJMP:
			{
				JMPBUF *thisjmp;
				jmp_val = popint();
				thisjmp = popptr();
				if (thisjmp->jmp_id != 0x0a0a)
					error(NOSETJMPERR);
				stmtjmp = *thisjmp;
				Ctx = stmtjmp.jmp_ctx;
				if (Ctx.NextVar->vprev)
					Ctx.NextVar->vprev->vnext = NULL;
				longjumping = TRUE;
				longjmp(stmtjmp.jb, 1);
				break;
			}
			default:	{
				static int vFuncs[] = {
					SYSSSCANF,
					SYSSPRINTF,
					SYSFSCANF,
					SYSFPRINTF,
					SYSSCANF,
					SYSPRINTF,
					SYSCPRINTF
				};
				for (i = 0; i < sizeof(vFuncs) / sizeof(int); i++)
					if (Ctx.Curfunction->libcode == vFuncs[i])
						break;
				if (i < sizeof(vFuncs) / sizeof(int))
					ArgumentList(argc, args);
				pushint(Ctx.Curfunction->libcode);
				inSystem = 1;
				if (setjmp(BreakJmp) == 0)
					sys();
				else 
					pushint(0);
				inSystem = 0;
				break;
			}
		}
		Saw_return = 1;
	}
	else	{
		/*
		 * Process the function's formal argument list and argument
		 * declaration list.
		 */
		ArgumentList(argc, args);
		/*
		 * beginning of statement for debugger
		 */
		Ctx.CurrFileno = Ctx.Curfunction->fileno;
		Ctx.CurrLineno = Ctx.Curfunction->lineno;
		stmtbegin();
		getoken();
		/*
		 * clear the arguments off the stack
		 */
		Ctx.Stackptr -= argc;

		/*
		 * execute the function
		 */
		Ctx.svpptr = Ctx.Progptr;
		statement();
	}
	/*
	 * If the function executed a return statement,
	 * the return value is whatever was last on the
	 * stack.  Otherwise, the function returns a value
	 * of zero.
	 */
	if (Saw_return)	{
		if (Ctx.Stackptr->lvalue)	{
			c = rslvsize(Ctx.Stackptr->size, Ctx.Stackptr->class);
			store(&Ctx.Stackptr->value.cval, c,
				Ctx.Stackptr->value.cptr, c, Ctx.Stackptr->isunsigned);
			Ctx.Stackptr->lvalue = 0;
		}
		Saw_return = 0;
	}
	else	{
		/* --- no return from function ---- */
		pushint(0);
		/* --- test for no return from function returning value --- */
		TestZeroReturn();
	}
	/*
	 * Return local variable space
	 */
	Ctx.NextData = Ctx.Curfunc->ldata;
	/*
	 * Restore caller's environment.
	 */
	Ctx.Progptr = svprogptr;
	Ctx.Curfunc = func.fprev;
	Ctx.Curfunction = func.fvar;
	getoken();	/* prepare for next statement */
}

/* --- test for no return from function returning value --- */
void TestZeroReturn(void)
{
	if (!Ctx.Curfunc->fvar->ismain)
		if (Ctx.Curfunc->fvar->type || Ctx.Curfunc->fvar->class)
			error(NULLRETERR);
}

static void ArgumentList(int argc, ITEM *args)
{
	VARIABLE *pvar;
	Assert(Ctx.Curfunction != NULL);
	pvar = Ctx.Curfunction->locals.vfirst;
	if (pvar != NULL)
		while (pvar->vkind & LABEL)
			pvar = pvar->vnext;
	Assert(Ctx.Curfunc != NULL);
	Ctx.Curfunc->ldata = Ctx.NextData;
	while (argc)	{
		void *adr = rslva(args->value.cptr, args->lvalue);
		int siz = rslvs(args->size, args->class);
		char *vdata;
		int rsiz = siz;

		/* --- if pvar is NULL or pvar -> auto variable,
			   there are more arguments than parameters --- */
		if (pvar != NULL)	{
			if (pvar->islocal == 2)
				rsiz = isPointer(pvar) ? sizeof(void *) : pvar->vwidth;
			if ((args->vconst & 2) && (pvar->vconst & 2) == 0)
				if (isPointer(pvar))
					error(CONSTARGERR);
		}
		/* ---- coerce chars to ints as arguments ---- */
		if (rsiz == sizeof(char))
			rsiz = sizeof(int);

		vdata = GetDataSpace(rsiz, 0);

		store(vdata, rsiz, adr, siz, args->isunsigned);

		Ctx.Curfunc->arglength += rsiz;

		if (pvar != NULL)
			pvar = pvar->vnext;
		++args;
		--argc;
	}
	GetDataSpace(Ctx.Curfunction->width, 0);
}


