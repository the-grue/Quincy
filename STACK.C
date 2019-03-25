/*
 * QUINCY Interpreter - stack routines
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qnc.h"

void psh(void);
static int incompatible(ITEM *, ITEM *);
static void numcheck(void);

#undef rslvaddr

static char *rslvaddr(char **addr, char lval)
{
	if (lval)	{
		if (ConstExpression)
			error(CONSTEXPRERR);
		return *addr;
	}
	return (char *)addr;
}

void assignment(void)
{
	/*
	 * Perform an assignment
	 */
	ITEM *sp;

	if ((sp = Ctx.Stackptr - 1) <= Stackbtm)
		error(POPERR);
	/*
	 * Make sure we've got an lvalue and compatible pointers
	 */
	if (readonly(sp))
		error(LVALERR);
	else if (incompatible(sp, Ctx.Stackptr))
		error(INCOMPATERR);
	else
	{
		/*
		 * Store the item, then pop it from stack.
		 */

		if (!SkipExpression)
			store(sp->value.cptr,
				  rslvsize(sp->size, sp->class),
				  rslvaddr(&Ctx.Stackptr->value.cptr,Ctx.Stackptr->lvalue),
				  rslvsize(Ctx.Stackptr->size,Ctx.Stackptr->class),
				  Ctx.Stackptr->isunsigned);
		pop();
		/*
		 * The result of an assignment operation is that
		 * the item on top of the stack inherits the
		 * attributes and value of the right-hand-operand,
		 * but is demoted to an RVALUE.
		 */
		torvalue(Ctx.Stackptr);
	}
}

void store(void *toaddr, int tosize, void *fraddr, int frsize, char isu)
{
	DATUM to;
	DATUM fr;
	int i;

	to.cptr = toaddr;
	fr.cptr = fraddr;

	if (tosize == sizeof(char))	{
		if (isu)	{
			if (frsize == sizeof(char))
				*to.ucptr = *fr.ucptr;
			else if (frsize == sizeof(int))
				*to.ucptr = *fr.uiptr;
			else if (frsize == sizeof(long))
				*to.ucptr = *fr.ulptr;
			else if (frsize == sizeof(double))
				*to.ucptr = *fr.fptr;
		}
		else	{
			if (frsize == sizeof(char))
				*to.cptr = *fr.cptr;
			else if (frsize == sizeof(int))
				*to.cptr = *fr.iptr;
			else if (frsize == sizeof(long))
				*to.cptr = *fr.lptr;
			else if (frsize == sizeof(double))
				*to.cptr = *fr.fptr;
		}
	}
	else if (tosize == sizeof(int))	{
		if (isu)	{
			if (frsize == sizeof(char))
				*to.uiptr = *fr.ucptr;
			else if (frsize == sizeof(int))
				*to.uiptr = *fr.uiptr;
			else if (frsize == sizeof(long))
				*to.uiptr = *fr.ulptr;
			else if (frsize == sizeof(double))
				*to.uiptr = *fr.fptr;
		}
		else {
			if (frsize == sizeof(char))
				*to.iptr = *fr.cptr;
			else if (frsize == sizeof(int))
				*to.iptr = *fr.iptr;
			else if (frsize == sizeof(long))
				*to.iptr = *fr.lptr;
			else if (frsize == sizeof(double))
				*to.iptr = *fr.fptr;
		}
	}
	else if (tosize == sizeof(long))	{
		if (isu)	{
			if (frsize == sizeof(char))
				*to.ulptr = *fr.ucptr;
			else if (frsize == sizeof(int))
				*to.ulptr = *fr.uiptr;
			else if (frsize == sizeof(long))
				*to.ulptr = *fr.ulptr;
			else if (frsize == sizeof(double))
				*to.ulptr = *fr.fptr;
		}
		else	{
			if (frsize == sizeof(char))
				*to.lptr = *fr.cptr;
			else if (frsize == sizeof(int))
				*to.lptr = *fr.iptr;
			else if (frsize == sizeof(long))
				*to.lptr = *fr.lptr;
			else if (frsize == sizeof(double))
				*to.lptr = *fr.fptr;
		}
	}
	else if (tosize == sizeof(double))	{
		if (isu)	{
			if (frsize == sizeof(char))
				*to.fptr = *fr.ucptr;
			else if (frsize == sizeof(int))
				*to.fptr = *fr.uiptr;
			else if (frsize == sizeof(long))
				*to.fptr = *fr.ulptr;
			else if (frsize == sizeof(double))
				*to.fptr = *fr.fptr;
		}
		else	{
			if (frsize == sizeof(char))
				*to.fptr = *fr.cptr;
			else if (frsize == sizeof(int))
				*to.fptr = *fr.iptr;
			else if (frsize == sizeof(long))
				*to.fptr = *fr.lptr;
			else if (frsize == sizeof(double))
				*to.fptr = *fr.fptr;
		}
	}
	else	{
		/* --- must be a struct or union --- */
		if (tosize != frsize)
			error(INCOMPTYPERR);
		for (i = 0; i < tosize; i++)
			*(to.cptr+i) = *(fr.cptr+i);
	}
}


void pop(void)
{
	/*
	 * Pop the stack and check for underflow
	 */
	if (Ctx.Stackptr-- <= Stackbtm)
		error(POPERR);
}

void psh(void)
{
	/*
	 * Advance stack pointer and check for overflow
	 */
	if (++Ctx.Stackptr > Stacktop)
		error(PUSHERR);
}

void popn(int argc)
{
	/*
	 * Pop the given number of arguments off the stack.
	 * Returns nothing useful.
	 */
	while (argc-- > 0)
		pop();
}

void push(char pkind, char isunsigned, char pclass, char plvalue,
		  unsigned int psize, char ptype,
		  VARIABLELIST *pelem, DATUM *pdatum, char pconst)
{
	/*
	 * Push item parts onto the stack
	 */
	int i;
	psh();
	Ctx.Stackptr->kind = pkind;
	Ctx.Stackptr->isunsigned = isunsigned;
	Ctx.Stackptr->class = pclass;
	Ctx.Stackptr->lvalue = plvalue;
	Ctx.Stackptr->size = psize;
	Ctx.Stackptr->type = ptype;
	Ctx.Stackptr->elem = pelem;
	Ctx.Stackptr->value = *pdatum;
	Ctx.Stackptr->vconst = pconst;
	Ctx.Stackptr->vqualifier = 0;
	Ctx.Stackptr->vstruct = NULL;
	for (i = 0; i < MAXDIM; i++)
		Ctx.Stackptr->dims[i] = 0;
}

void pushint(int ivalue)
{
	/*
	 * push an integer onto the stack
	 */
	psh();
	Ctx.Stackptr->kind =
	Ctx.Stackptr->isunsigned =
	Ctx.Stackptr->class =
	Ctx.Stackptr->vconst =
	Ctx.Stackptr->vqualifier =
	Ctx.Stackptr->lvalue = 0;
	Ctx.Stackptr->elem = NULL;
	Ctx.Stackptr->size = sizeof(int);
	Ctx.Stackptr->type = INT;
	Ctx.Stackptr->value.ival = ivalue;
}

void pushptr(void *pvalue, char ptype)
{
	/*
	 * push a pointer onto the stack
	 */
	psh();
	Ctx.Stackptr->kind = 0;
	Ctx.Stackptr->isunsigned = 0;
	Ctx.Stackptr->class = 1;
	Ctx.Stackptr->vqualifier = 0;
	Ctx.Stackptr->vconst = 0;
	Ctx.Stackptr->lvalue = 0;
	Ctx.Stackptr->elem = NULL;
	Ctx.Stackptr->size = TypeSize(ptype);
	if (Ctx.Stackptr->size == 0)
		Ctx.Stackptr->size = sizeof(void *);
	Ctx.Stackptr->type = ptype;
	Ctx.Stackptr->value.cptr = pvalue;
}

void pushlng(long lvalue)
{
	/*
	 * push a long integer onto the stack
	 */
	psh();
	Ctx.Stackptr->kind = 
	Ctx.Stackptr->isunsigned =
	Ctx.Stackptr->class =
	Ctx.Stackptr->vconst =
	Ctx.Stackptr->vqualifier =
	Ctx.Stackptr->lvalue = 0;
	Ctx.Stackptr->elem = NULL;
	Ctx.Stackptr->size = sizeof(long);
	Ctx.Stackptr->type = LONG;
	Ctx.Stackptr->value.lval = lvalue;
}

void pushflt(double fvalue)
{
	/*
	 * push a floating point number onto the stack
	 */
	psh();
	Ctx.Stackptr->kind = 
	Ctx.Stackptr->isunsigned =
	Ctx.Stackptr->class =
	Ctx.Stackptr->vconst =
	Ctx.Stackptr->vqualifier =
	Ctx.Stackptr->lvalue = 0;
	Ctx.Stackptr->elem = NULL;
	Ctx.Stackptr->type = FLOAT;
	Ctx.Stackptr->size = sizeof(double);
	Ctx.Stackptr->value.fval = fvalue;
}

int popnint(int argc)
{
	/*
	 * Pop the given number of arguments off the stack.
	 * Returns the integer value of the first item popped.
	 */
	int ivalue;

	ivalue = popint();
	popn(--argc);

	return ivalue;
}

int popint(void)
{
	/*
	 * Resolve the item on the top of the stack and return it
	 * as an integer.
	 */
	int ivalue;

	numcheck();
	store(&ivalue, sizeof(int),
		rslvaddr(&Ctx.Stackptr->value.cptr, Ctx.Stackptr->lvalue),
		rslvsize(Ctx.Stackptr->size, Ctx.Stackptr->class),
		Ctx.Stackptr->isunsigned
	);
	pop();
	return ivalue;
}

void *popptr(void)
{
	/*
	 * Resolve the item on the top of the stack and return it
	 * as a pointer.
	 */
	void *pvalue;

	if (Ctx.Stackptr->class == 0 && Ctx.Stackptr->value.ival)
		error(ADDRERR);
	store(&pvalue, sizeof(void *),
		rslvaddr(&Ctx.Stackptr->value.cptr, Ctx.Stackptr->lvalue),
		rslvsize(Ctx.Stackptr->size, Ctx.Stackptr->class),
		0
	);
	pop();
	return pvalue;
}

long poplng(void)
{
	/*
	 * Resolve the item on the top of the stack and return it
	 * as a long integer.
	 */
	long lvalue;

	numcheck();
	store(&lvalue, sizeof(long),
		rslvaddr(&Ctx.Stackptr->value.cptr, Ctx.Stackptr->lvalue),
		rslvsize(Ctx.Stackptr->size, Ctx.Stackptr->class),
		Ctx.Stackptr->isunsigned
	);
	pop();
	return lvalue;
}

double popflt(void)
{
	/*
	 * Resolve the item on the top of the stack and return it
	 * as a floating point number.
	 */
	double fvalue;

	numcheck();
	store(&fvalue, sizeof(double),
		rslvaddr(&Ctx.Stackptr->value.cptr, Ctx.Stackptr->lvalue),
		rslvsize(Ctx.Stackptr->size, Ctx.Stackptr->class),
		Ctx.Stackptr->isunsigned
	);
	pop();
	return fvalue;
}

void topget(ITEM *pitem)
{
	/*
	 * Get the attributes of the item on top of the stack
	 * This routine is used by the add() function to save
	 * the attributes of a pointer variable on the stack
	 * before addition or subtraction of pointers.
	 */
	int i;
	pitem->kind = Ctx.Stackptr->kind;
	pitem->isunsigned = Ctx.Stackptr->isunsigned;
	pitem->class = Ctx.Stackptr->class;
	pitem->size = Ctx.Stackptr->size;
	pitem->type = Ctx.Stackptr->type;
	pitem->elem = Ctx.Stackptr->elem;
	pitem->vconst = Ctx.Stackptr->vconst;
	pitem->vqualifier = Ctx.Stackptr->vqualifier;
	for (i = 0; i < MAXDIM; i++)
		pitem->dims[i] = Ctx.Stackptr->dims[i];
}

void topset(ITEM *pitem)
{
	/*
	 * Set the attributes of the item on top of the stack
	 * This routine is used by the add() function to duplicate
	 * the attributes of a pointer variable on the stack
	 * after addition or subtraction of pointers.
	 */
	int i;
	Ctx.Stackptr->kind = pitem->kind;
	Ctx.Stackptr->isunsigned = pitem->isunsigned;
	Ctx.Stackptr->class = pitem->class;
	Ctx.Stackptr->lvalue = 0;	/* the item always turns into an RVALUE */
	Ctx.Stackptr->type = pitem->type;
	Ctx.Stackptr->size = pitem->size;
	Ctx.Stackptr->elem = pitem->elem;
	Ctx.Stackptr->vconst = pitem->vconst;
	Ctx.Stackptr->vqualifier = pitem->vqualifier;
	for (i = 0; i < MAXDIM; i++)
		Ctx.Stackptr->dims[i] = pitem->dims[i];
}

void topdup(void)
{
	/*
	 * Push a duplicate of the item on top of the stack.
	 */
	push(Ctx.Stackptr->kind, Ctx.Stackptr->isunsigned,
		 Ctx.Stackptr->class, Ctx.Stackptr->lvalue,
		 Ctx.Stackptr->size, Ctx.Stackptr->type,
		 Ctx.Stackptr->elem, &Ctx.Stackptr->value,
		 Ctx.Stackptr->vconst);
}

int readonly(ITEM *sp)
{
	return (
		(!sp->class && sp->vconst)
			||
		(sp->class && (sp->vconst & 1))
			||
		(!sp->lvalue)
	);
}

static int incompatible(ITEM *d, ITEM *s)
{
	if (d->class == 0)
		return 0;
	if (d->vconst & 1)
		return 1;
	if (d->vconst == 2)
		return 0;
	return (s->vconst > 1);
}

int StackItemisNumericType(void)
{
	char type = Ctx.Stackptr->type;
	return (StackItemisAddressOrPointer() ||
		!(type == STRUCT || type == VOID || type == UNION
			|| (Ctx.Stackptr->kind & FUNCT)));
}

static void numcheck(void)
{
	/*
	 * Check for numeric type
	 */
	if (!StackItemisNumericType())
		error(NOTNUMERR);
}

void torvalue(ITEM *item)
{
	/*
	 * Convert the item pointed at by "item" to an RVALUE.
	 */
	int i;

	if (item->lvalue)	{
		i = rslvsize(item->size, item->class);
		store(&item->value.cval, i, item->value.cptr, i, item->isunsigned);
		item->lvalue = 0;
	}
}

void FixStackType(char typ)
{
	switch (typ)	{
		case CHAR:
		case INT:
			pushint(popflt());
			break;
		case LONG:
			pushlng(popflt());
			break;
		default:
			break;
	}
}

