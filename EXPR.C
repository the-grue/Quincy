/*
 * QUINCY Interpreter - parser module: expressions
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "qnc.h"

static char Opassgn;
static void assgn(void);
static void logic1(void);
static void logic2(void);
static void bool1(void);
static void bool2(void);
static void bool3(void);
static void reln1(void);
static void reln2(void);
static void shift(void);
static void add(void);
static void mult(void);
static void classcheck(void);
static void skipexp(void (*fn)(void));

int SkipExpression;

int ExpressionOne()
{
	int argc;
	if ((argc = expression()) > 1)	{
		/*
		 * an expression statement like this
		 *      ++argv, --argc;
		 * leaves the right-most expression
		 * (--argc) on the stack when it's done.
		 */
		ITEM top = *Ctx.Stackptr;
		popn(argc);
		*++Ctx.Stackptr = top;
		argc = 1;
	}
	return argc;
}
		
int expression()
{
	/*
	 * Parse a comma separated expression.  Returns a count of
	 * the number of subexpressions.
	 */
	ITEM *sp;
	unsigned stat87;

	sp = Ctx.Stackptr;
	cond();
	while (Ctx.Token == T_COMMA)	{
		if (Ctx.Stackptr-sp)	{
			getoken();
			if (Ctx.Token == T_SEMICOLON)
				error(COMMAERR);
			else
				cond();
		}
		else
			error(COMMAERR);
	}
	stat87 = _status87();
	if (stat87 & 0x5d)	{
		_clear87();
		error(MATHERR);
	}
	return Ctx.Stackptr - sp;
}

void cond()
{
	/*
	 * Parse a conditional expression; <expr> ? <expr> : <expr>
	 */
	assgn();
	if (Ctx.Token == T_COND)	{
		getoken();
		if (SkipExpression)	{
			cond();
			if (Ctx.Token != T_COLON)
				error(COLONERR);
			getoken();
			cond();
			return;
		}
		else if (popint())	{
			cond();
			if (Ctx.Token == T_COLON)	{
				getoken();
				skipexp(cond);
			}
			else
				error(COLONERR);
		}
		else	{
			skipexp(cond);
			if (Ctx.Token == T_COLON)	{
				getoken();
				cond();
			}
			else
				error(COLONERR);
		}
	}
}

static void assgn()
{
	/*
	 * Parse an assignment expression.	 
	 */

	logic1();
	/*
	 * if there's an '=', perform the assignment
	 */
	if (Ctx.Token==T_ASSIGN)	{
		getoken();
		cond();
		if (!SkipExpression)
			assignment();
	}
	else if (Ctx.Token & 128)	{
		/*
		 * It's an <op-assignment> operator.
		 */
		unsigned char svtoken, optoken;
		ITEM *svstackptr;

		if (SkipExpression)	{
			getoken();
			cond();
			return;
		}

		optoken = Ctx.Token & 127;
		/*
		 * We're going to be fiddling with the stack, so keep
		 * track of current top of stack then duplicate the
		 * item that's currently on top.
		 */
		svstackptr = Ctx.Stackptr;
		topdup();

		/*
		 * Now parse another expression as if nothing happened.
		 */
		getoken();
		cond();

		/*
		 * Traverse down through the operator handlers to the
		 * one that does the <op> part of <op-assignment>'s
		 */
		svtoken = Ctx.Token;
		Ctx.Token = optoken;
		/*
		 * Tell the operator handlers that we're doing an
		 * <op-assignment> by setting the "Opassgn" variable.
		 * If for example we had parsed the expression:
		 *    "var += 2";
		 * the stack would look like this:
		 *
		 *     2    <- current top of stack (Ctx.Stackptr)
		 *    var
		 *    var   <- bottom of stack
		 */
		++Opassgn;
		Ctx.Stackptr -= 2;
		bool1();
		--Opassgn;
		Ctx.Token = svtoken;
		/*
		 * Now do the assignment and restore the stack.
		 */
		assignment();
		Ctx.Stackptr = svstackptr;
	}
}

static int BinarySkip(void (*fn)(void))
{
	if (SkipExpression)	{
		Ctx.Token = 0;
		if (!Opassgn)
			getoken();
		(*fn)();
	}
	return SkipExpression;
}

static void logic1()
{
	/*
	 * Logical OR
	 */
	logic2();
	while (Ctx.Token==T_LIOR)	{
		if (BinarySkip(logic2))
			continue;
		getoken();
		if (popint())	{
			/*
			 * Expression is already true - skip
			 * to end of expression
			 */
			skipexp(logic2);
			pushint(1);
		}
		else	{
			logic2();
			pushint(!!popint());
		}
	}
}

static void logic2()
{
	/*
	 * Logical AND
	 */
	bool1();
	while (Ctx.Token==T_LAND)	{
		if (BinarySkip(bool1))
			continue;
		getoken();
		if (popint())	{
			bool1();
			pushint(!!popint());
		}
		else	{
			/*
			 * Expression is already false - skip
			 * to end of expression.
			 */
			skipexp(bool1);
			pushint(0);
		}
	}
}

static void skipexp(void (*fn)(void))
{
	SkipExpression++;
	(*fn)();
	--SkipExpression;
}

static void NeedIntegerType(ITEM *item)
{
	if (!SkipExpression && !ItemisInteger(item))
		error(INTTYPERR);
}

static void bool1()
{
	/*
	 * Binary OR
	 */

	bool2();
	while (Ctx.Token == T_IOR)	{
		NeedIntegerType(Ctx.Stackptr);
		if (Opassgn)
			Ctx.Token = 0;
		else
			getoken();
		bool2();
		NeedIntegerType(Ctx.Stackptr);
		if (!SkipExpression)
			pushint(popint() | popint());
	}
}

static void bool2()
{
	/*
	 * Binary XOR
	 */

	bool3();
	while (Ctx.Token == T_XOR)	{
		NeedIntegerType(Ctx.Stackptr);
		if (Opassgn)
			Ctx.Token = 0;
		else
			getoken();
		bool3();
		NeedIntegerType(Ctx.Stackptr);
		if (!SkipExpression)
			pushint(popint() ^ popint());
	}
}

static void bool3()
{
	/*
	 * Binary AND
	 */

	reln1();
	while (Ctx.Token == T_AND)	{
		NeedIntegerType(Ctx.Stackptr);
		if (Opassgn)
			Ctx.Token = 0;
		else
			getoken();
		reln1();
		NeedIntegerType(Ctx.Stackptr);
		if (!SkipExpression)
			pushint(popint() & popint());
	}
}

static void reln1()
{
	/*
	 * Equality operators
	 */
	reln2();

	for (;;)	{
		if (Ctx.Token == T_EQ)	{
			getoken();
			reln2();
			if (!SkipExpression)
				pushint((int)(popflt() == popflt()));
		}
		else if (Ctx.Token == T_NE)	{
			getoken();
			reln2();
			if (!SkipExpression)
				pushint((int)(popflt() != popflt()));
		}
		else
			return;
	}
}

static int CompareItems(double *fval)
{
	ITEM left;
	if (BinarySkip(shift))
		return 4;
	getoken();
	left = *Ctx.Stackptr;
	*fval = popflt();
	shift();
	if (left.class != Ctx.Stackptr->class)
		error(PTRCOMPERR);
	if (left.class || (left.isunsigned && Ctx.Stackptr->isunsigned))
		return 0;
	else if (left.isunsigned)
		return 1;
	else if (Ctx.Stackptr->isunsigned)
		return 2;
	else
		return 3;
}

static void reln2()
{
	/*
	 * Relational operators.  Note: this module knows that unsigned
	 * comparisons should be made when pointers or unsigned ints
	 * are involved.
	 */
	double fval;

	shift();

	for (;;)	{
		switch (Ctx.Token)	{
			case T_LE:
				switch (CompareItems(&fval))	{
					case 0:
						pushint((unsigned)fval <= (unsigned)popint());
						break;
					case 1:
						pushint((unsigned)fval <= popflt());
						break;
					case 2:
						pushint(fval <= (unsigned)popint());
						break;
					case 3:
						pushint(fval <= popflt());
						break;
				}
				break;
			case T_GE:
				switch (CompareItems(&fval))	{
					case 0:
						pushint((unsigned)fval >= (unsigned)popint());
						break;
					case 1:
						pushint((unsigned)fval >= popflt());
						break;
					case 2:
						pushint(fval >= (unsigned)popint());
						break;
					case 3:
						pushint(fval >= popflt());
						break;
				}
				break;
			case T_LT:
				switch (CompareItems(&fval))	{
					case 0:
						pushint((unsigned)fval < (unsigned)popint());
						break;
					case 1:
						pushint((unsigned)fval < popflt());
						break;
					case 2:
						pushint(fval < (unsigned)popint());
						break;
					case 3:
						pushint(fval < popflt());
						break;
				}
				break;
			case T_GT:
				switch (CompareItems(&fval))	{
					case 0:
						pushint((unsigned)fval > (unsigned)popint());
						break;
					case 1:
						pushint((unsigned)fval > popflt());
						break;
					case 2:
						pushint(fval > (unsigned)popint());
						break;
					case 3:
						pushint(fval > popflt());
						break;
				}
				break;
			default:
				return;
		}
	}
}

static void shift()
{
	/*
	 * Arithmetic shift left and right operators
	 */
	int ival;

	add();
	for (;;)	{
		switch (Ctx.Token)	{
			case T_SHL:
				NeedIntegerType(Ctx.Stackptr);
				ival = popint();
				if (Opassgn)	{
					++Ctx.Stackptr;
					Ctx.Token = 0;
				}
				else
					getoken();
				add();
				NeedIntegerType(Ctx.Stackptr);
				if (!SkipExpression)
					pushint(ival << popint());
				break;
			case T_SHR:
				NeedIntegerType(Ctx.Stackptr);
				ival = popint();
				if (Opassgn)	{
					++Ctx.Stackptr;
					Ctx.Token = 0;
				}
				else
					getoken();
				add();
				NeedIntegerType(Ctx.Stackptr);
				if (!SkipExpression)
					pushint(ival >> popint());
			default:
				return;
		}
	}
}

static void add()
{
	/*
	 * Addition and subtraction.  This function knows about pointers.
	 */
	ITEM p1, p2;
	int ival, size1, size2;
	char *pval;
	double fval;

	mult();
	for (;;)	{
		switch (Ctx.Token)	{
		case T_ADD:
			/*
			 * Save characteristics of first operand
			 */
			if (BinarySkip(mult))
				continue;
			topget(&p1);
			if (ItemisAddressOrPointer(p1))	{
				if (p1.type == VOID)
					error(VOIDPTRERR);
				/*
				 * Watch out for pointers to pointers:
				 * we have to add the size of a pointer
				 * (not the size of the fundamental
				 * data type) to the base address.
				 */
				size1 = ElementWidth(&p1);
				pval = popptr();
			}
			else
				fval = popflt();
			if (Opassgn)	{
				++Ctx.Stackptr;
				Ctx.Token = 0;
			}
			else
				getoken();
			mult();
			/*
			 * Save characteristics of second operand
			 */
			topget(&p2);
			if (ItemisAddressOrPointer(p2))	{
				if (p2.type == VOID)
					error(VOIDPTRERR);
				size2 = ElementWidth(&p2);
			}
			if (ItemisAddressOrPointer(p1) && ItemisAddressOrPointer(p2))
				/*
				 * Addition of pointers is not
				 * allowed, according to K&R
				 */
				error(PTROPERR);
			else if (ItemisAddressOrPointer(p1))	{
				NeedIntegerType(Ctx.Stackptr);
				pushptr(pval + popint()*size1, p1.type);
				topset(&p1);
			}
			else if (ItemisAddressOrPointer(p2))	{
				NeedIntegerType(&p1);
				ival = fval;
				pushptr(ival*size2 + (char *)popptr(), p2.type);
				topset(&p2);
			}
			else	{
				pushflt(fval + popflt());
				FixStackType(p1.type);
			}
			break;
		case T_SUB:
			/*
			 * Save characteristics of first operand
			 */
			if (BinarySkip(mult))
				continue;
			topget(&p1);
			if (ItemisAddressOrPointer(p1))	{
				if (p1.type == VOID)
					error(VOIDPTRERR);
				size1 = ElementWidth(&p1);
				pval = popptr();
			}
			else
				fval = popflt();
			if (Opassgn)	{
				++Ctx.Stackptr;
				Ctx.Token = 0;
			}
			else
				getoken();
			mult();
			/*
			 * Save characteristics of second operand
			 */
			topget(&p2);
			if (ItemisAddressOrPointer(p2))	{
				if (p2.type == VOID)
					error(VOIDPTRERR);
				size2 = ElementWidth(&p2);
			}
			if (ItemisAddressOrPointer(p1) && ItemisAddressOrPointer(p2))	{
				/*
				 * Subtraction of pointers is allowed,
				 * but only if they point to the same
				 * type of object.  The result is an
				 * integer object.
				 */
				if (p1.class != p2.class || p1.type != p2.type)
					error(PTROPERR);
				else
					pushint((pval - (char *)popptr())/size1);
			}
			else if (ItemisAddressOrPointer(p1))	{
				NeedIntegerType(Ctx.Stackptr);
				pushptr(pval - popint()*size1, p1.type);
				topset(&p1);
			}
			else if (ItemisAddressOrPointer(p2))	{
				/*
				 * Can't subtract a pointer from an int:
				 *    int i, char *cp;  i - cp;
				 */
				error(PTROPERR);
			}
			else	{
				pushflt(fval - popflt());
				FixStackType(p1.type);
			}
			break;
		default:
			return;
		}
	}
}

static void mult()
{
	/*
	 * Evaluate a multiplicative series
	 */
	double fval, fval2;
	double md;
	unsigned long int f, f2;
	char stktype;

	if (Opassgn)
		++Ctx.Stackptr;
	else
		primary();

	for (;;)	{
		switch (Ctx.Token)	{
		case T_MUL:
			if (BinarySkip((void(*)())primary))
				continue;
			classcheck();
			stktype = Ctx.Stackptr->type;
			fval = popflt();
			if (Opassgn)	{
				Ctx.Stackptr += 2;
				Ctx.Token = 0;
			}
			else	{
				getoken();
				primary();
			}
			classcheck();
			pushflt(fval * popflt());
			FixStackType(stktype);
			break;
		case T_DIV:
			if (BinarySkip((void(*)())primary))
				continue;
			classcheck();
			stktype = Ctx.Stackptr->type;
			fval = popflt();
			if (Opassgn)	{
				Ctx.Stackptr += 2;
				Ctx.Token = 0;
			}
			else	{
				getoken();
				primary();
			}
			classcheck();
			if ((fval2 = popflt()) != 0)
				pushflt(fval / fval2);
			else
				error(DIV0ERR);
			FixStackType(stktype);
			break;
		case T_MOD:
			if (BinarySkip((void(*)())primary))
				continue;
			classcheck();
			stktype = Ctx.Stackptr->type;
			fval = popflt();
			if (Opassgn)	{
				Ctx.Stackptr += 2;
				Ctx.Token = 0;
			}
			else	{
				getoken();
				primary();
			}
			classcheck();
			fval2 = popflt();
			if ((int)fval2)	{
				f = (unsigned long int) fval;
				f2 = (unsigned long int) fval2;
				md = ((double) (f % f2));
				pushflt(md);
			}
			else
				error(DIV0ERR);
			FixStackType(stktype);
			break;
		default:
			return;
		}
	}
}

static void classcheck()
{
	/*
	 * Check for illegal pointer operation
	 */
	if (Ctx.Stackptr->class)
		error(PTROPERR);
}

