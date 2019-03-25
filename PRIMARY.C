/*
 * QUINCY Interpreter - parser module: primary
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qnc.h"

static void postop(void);
static void prepostop(int, char);
static VARIABLE *element(void);
extern int SteppingOver;
extern int StepCount;

int ItemArrayDimensions(ITEM *item)
{
	int i;

	for (i = MAXDIM; i > 0; --i)
		if (item->dims[i-1])
			break;
	return i;
}

int ItemArrayElements(ITEM *item)
{
	int i, j = 1;
	for (i = 0; i < item->class; i++)
		if (item->dims[i])
			j *= item->dims[i];
	return j;
}

int ElementWidth(ITEM *item)
{
	int rtn = sizeof(void*);
	int off = 1;
	int j = item->class;
	int m;
	if (ItemisArray(*item) || item->dims[1])	{
		int k = ItemArrayDimensions(item);
		if (j <= k)	{
			while (--j)	{
				m = item->dims[--k];
				off *= m ? m : 1;
			}
			rtn = item->size * off;
		}
	}
	else if (j < 2)
		rtn = item->size;
	return rtn;
}
static int ComputeDimension(int dim)
{
	int width;
	if (dim == MAXDIM)
		error(MDIMERR);
	getoken();
	/*
	 * compute the subscript
	 */
	expression();
	if (Ctx.Token!=T_RBRACKET)
		error(RPARENERR);
	getoken();
	width = popint();

	/*
	 * Compute the offset (subscript times the
	 * size of the thing the pointer is pointing
	 * at) and then the effective address.
	 */

	width *= ElementWidth(Ctx.Stackptr);
	return width;
}

static void Subscript(int dim)
{
	int width;
	int dims = ItemArrayDimensions(Ctx.Stackptr);
	width = ComputeDimension(dim);
	if (Ctx.Stackptr->lvalue && dim == 0)
	    /*
 	 	 * pointer
	 	 * the stack top item's value is the
	 	 * address of the pointer, so
	 	 * do another level of indirection.
	 	 */
		Ctx.Stackptr->value.pptr =(char **)(*Ctx.Stackptr->value.pptr);
	Ctx.Stackptr->value.cptr += width;

	/*
	 * The stack top item becomes an LVALUE,
	 * with reduced indirection level.
	 */

	--Ctx.Stackptr->class;
	if (Ctx.Stackptr->class == dims ||
			Ctx.Stackptr->class == 0 ||
				!ItemisArray(*Ctx.Stackptr))
		Ctx.Stackptr->lvalue = 1;
}

VARIABLE *primary()
{
	/*
	 * Parse a primary.  This function handles the following
	 * left-to-right associative operators:
	 *
	 *    ()  function call
	 *    []  array reference (only single dimension arrays)
	 *    ->  structure pointer member selection
	 *    .   structure member selection
	 */
	VARIABLE var, *pvar;
	VARIABLELIST svstruct;
	char c;
	int done = 0;
	int dim = 0;

	pvar = element();

	while (!done)	{
		switch (Ctx.Token)	{
			case T_LBRACKET:
				if (SkipExpression)	{
					skip(T_LBRACKET, T_RBRACKET);
					break;
				}
				/*
				 * If it's not an address it can't be used as an
				 * array, but the pointer operator is still allowed.
				 */
				if (!StackItemisAddressOrPointer())
					error(NOTARRAYERR);
				if (Ctx.Stackptr->type == VOID)
					error(VOIDPTRERR);
				while (Ctx.Token == T_LBRACKET)
					Subscript(dim++);
				dim = 0;
				break;
		
			case T_LPAREN:
				if (SkipExpression)	{
					skip(T_LPAREN, T_RPAREN);
					break;
				}
				if (!(Ctx.Stackptr->kind & FUNCT))
					error(FUNCERR);
				Ctx.Curfunction = popptr();
				Assert(Ctx.Curfunction != NULL);
				if (ConstExpression)
					error(CONSTEXPRERR);
				/*
			 	 * function call
			 	 */
				if (SteppingOver)
					StepCount++;
				callfunc();
				if (SteppingOver)
					--StepCount;
				break;
		
			case T_ARROW:
				/*
			 	 * Union or Structure pointer reference
			 	 */
				if (ConstExpression)
					error(CONSTEXPRERR);
				if (!SkipExpression)	{
					if (!StackItemisAddressOrPointer())
						/*
				 		* not a pointer
				 		*/
						error(STRUCTPTRERR);
					if (Ctx.Stackptr->lvalue)
						Ctx.Stackptr->value.cptr =
							*Ctx.Stackptr->value.pptr;
					/*
			 	 	* item on top of the stack is
			 	 	* promoted to LVALUE, and its
			 	 	* indirection level decreases.
			 	 	*/
					Ctx.Stackptr->lvalue = 1;
					--Ctx.Stackptr->class;
					/*
			 	 	* same as a struct.elem reference, fall into next case
			 	 	*/
				}
			case T_DOT:	{
				ITEM elem;

				if (!SkipExpression)	{
					if (Ctx.Token == T_DOT && StackItemisAddressOrPointer())
						error(PTROPERR);
					/*
			 	 	* Union or Structure element reference
			 	 	*/
					if (!Ctx.Stackptr->elem)
						/*
				 	 	* Thing on stack is not a structure
				 	 	*/
						error(STRUCERR);

				}

				/* --- Ctx.Curstruct supports the debugger --- */
				svstruct = Ctx.Curstruct;
				Ctx.Curstruct = *Ctx.Stackptr->elem;
				getoken();
				pvar = element();
				Ctx.Curstruct = svstruct;

				if (SkipExpression)
					break;

				if (!(pvar->vkind & STRUCTELEM))
					error(ELEMERR);
				elem = *Ctx.Stackptr;
				/*
				 * pop the element
			 	 */
				pop();
				/*
				 * replace the structure with the element
				 */
				*Ctx.Stackptr = elem;
				postop();
				break;
			}		
			default:
				done = 1;
				break;
		}
	}
	return pvar;
}

static VARIABLE *element()
{
	/*
	 * Evaluate an element
	 */
	ITEM *svstackptr;
	static VARIABLE *pvar = NULL;
	int i, csize, ctype;
	char cclass, stktype;
	VARIABLE cvar;

	if (Ctx.Token == T_LPAREN)	{
		if (SkipExpression)	{
			skip(T_LPAREN, T_RPAREN);
			return NULL;
		}
		getoken();
		if (istypespec() || Ctx.Token == T_VOID)	{
			/*
			 * typecast.
			 */
			cvar = *DeclareVariable(0, 0, 0, 0, 0);

			if (Ctx.Token==T_RPAREN)
				getoken();
			else
				error(RPARENERR);
			svstackptr = Ctx.Stackptr;
			pvar = primary();
			if (SkipExpression)
				return pvar;
			if (Ctx.Stackptr == svstackptr)
				/*
				 * no primary after the
				 * typecast; push null variable
				 * to keep the stack straight.
				 */
				push(0, 0, 0, 0, 0, 0, 0, &Ctx.Value, 0);
			else	{
				/*
				 * Cast item on stack to new type.
				 */
				if (cvar.vtype != VOID || cvar.vclass)
					store(
						rslvaddr(&Ctx.Stackptr->value.cptr,
							Ctx.Stackptr->lvalue),
						rslvsize(cvar.vsize, cvar.vclass),
						rslvaddr(&Ctx.Stackptr->value.cptr,
							Ctx.Stackptr->lvalue),
						rslvsize(Ctx.Stackptr->size,
							Ctx.Stackptr->class),
						cvar.isunsigned
					);
				Ctx.Stackptr->class = cvar.vclass;
				Ctx.Stackptr->size = cvar.vsize;
				Ctx.Stackptr->type = cvar.vtype;
				Ctx.Stackptr->vstruct = cvar.vstruct;
				for (i = 0; i < MAXDIM; i++)
					Ctx.Stackptr->dims[i] = cvar.vdims[i];
			}
			return pvar;
		}
		/*
		 * it's a parenthesized expression
		 */
		ExpressionOne();		/* then do an expression */
		if (Ctx.Token==T_RPAREN)	{ 	/* look for a ')' */
			getoken();
			postop();
			return pvar;
		}
		error(RPARENERR);
	}

	pvar = NULL;

	switch (Ctx.Token)	{
		case T_LNOT:
			/*
		 	 * Logical NOT
		 	 *
		 	 * NOTE: All arithmetic and logical unary operators
		 	 * turn the item currently on the top of the stack
		 	 * into an integer RVALUE.  So, it's safe to just
		 	 * to say: pushint(<unary_operator> popint());
		 	 */
			getoken();
			pvar = primary();
			if (!SkipExpression)
				pushint(! popint());
			break;
		case T_NOT:
			/*
		 	 * One's complement
		 	 */
			getoken();
			primary();
			if (!SkipExpression)
				pushint(~popint());
			break;
		case T_ADD:
			/*
		 	 * Unary positive operator
		 	 */
			getoken();
			primary();
			break;
		case T_SUB:
			/*
		 	 * Negation (two's complement)
		 	 */
			getoken();
			primary();
			stktype = Ctx.Stackptr->type;
			if (!SkipExpression)
				pushflt(-popflt());
			FixStackType(stktype);
			break;
		case T_INCR:
			getoken();
			primary();
			prepostop(0, T_INCR);
			break;
		case T_DECR:
			getoken();
			primary();
			prepostop(0, T_DECR);
			break;
		case T_PTR:
			/*
		 	 * Pointer operator.
		 	 */
			if (ConstExpression)
				error(CONSTEXPRERR);
			getoken();
			primary();
			if (SkipExpression)
				break;
			if ((cclass = Ctx.Stackptr->class) == 0)
				error(NOTPTRERR);
			if ((Ctx.Stackptr->kind & FUNCT) == 0)	{
				if (Ctx.Stackptr->type == VOID)
					error(VOIDPTRERR);
				/*
			 	 * indirection level (class) decreases.
			 	 */
				Ctx.Stackptr->class = --cclass;
				Ctx.Stackptr->vconst = (Ctx.Stackptr->vconst >> 1);
				/*
			 	 * If item on stack is already an LVALUE,
				 * do an extra level of indirection
			 	 */
				if (Ctx.Stackptr->lvalue)
					Ctx.Stackptr->value.pptr =
						(char **)*Ctx.Stackptr->value.pptr;
				if (cclass == 0 || !ItemisArray(*Ctx.Stackptr))
					Ctx.Stackptr->lvalue = 1;
			}
			break;
		case T_AND:
			/*
		 	 * Address operator.
		 	 */
			getoken();
			primary();
			if ((Ctx.Stackptr->vqualifier) & REGISTER)
				error(REGADDRERR);
			if (SkipExpression)
				break;
			/*
		 	 * The item must be an LVALUE (can't take
		 	 * the address of a constant, now can we?)
		 	 */

			if (!Ctx.Stackptr->lvalue 					&&
				Ctx.Stackptr->elem == 0					&&
				Ctx.Stackptr->class == 0)

				error(LVALERR);
			/*
			 * inverse of the pointer operator
			 */
			if ((Ctx.Stackptr->kind & FUNCT) == 0)	{
				Ctx.Stackptr->lvalue = 0;
				Ctx.Stackptr->class++;
				Ctx.Stackptr->vconst <<= 1;
			}
			/*
		 	 * The item on stack is now an address (pointer).
		 	 */
			break;
		case T_SIZEOF:
			/*
		 	 * Sizeof operator.
		 	 */
			getoken();

			if (Ctx.Token == T_LPAREN)	{
				getoken();
				if (istypespec())	{
					/*
				 	 * It's a data type specifier.
				 	 */
					pvar = DeclareVariable(0, 0, 0, 0, 0);
					csize = pvar->vsize;
					ctype = pvar->vtype;
					cclass = pvar->vclass;
					if (!SkipExpression)
						push(pvar->vkind, pvar->isunsigned,
							cclass, cclass, csize, ctype, 0, &Ctx.Value, 0);
				}
				else
					/*
				 	 * Otherwise it's a parenthesized expression.
				 	 */
					primary();
				if (Ctx.Token==T_RPAREN)
					getoken();
				else
					error(RPARENERR);
			}
			else
				primary();

			pvar->vconst = 0;
			if (SkipExpression)
				break;
			if (StackItemisAddressOrPointer())	{
				/*
			 	 * it's a pointer or an array.
			 	 */
				if (Ctx.Stackptr->lvalue)
					/*
				 	 * pointer
				 	 */
					i = sizeof(void *);
				else	{
					/*
				 	 * array
				 	 */
					i = Ctx.Stackptr->size;
					if (pvar && isArray(pvar))
						i *= ArrayElements(pvar);
					else if (Ctx.Stackptr->size == 1)
						/*
					 	 * it's a string constant
					 	 */
						i = strlen(Ctx.Stackptr->value.cptr) + 1;
				}
			}
			else
				i = Ctx.Stackptr->size;

			pop();
			pushint(i);
			break;
		case T_CHRCONST:
			/*
		 	 * A character constant
		 	 */
			if (!SkipExpression)	{
				pushint(Ctx.Value.ival);
				/*
		 	 	* Change the size of the item on the stack to CHAR
		 	 	*/
				Ctx.Stackptr->size = sizeof(char);
				/*
		 	 	* byte order problem
		 	 	*/
				Ctx.Stackptr->value.cval = Ctx.Stackptr->value.ival;
			}
			getoken();
			break;
		case T_INTCONST:
			/*
		 	 * A numeric constant
		 	 */
			if (!SkipExpression)
				pushint(Ctx.Value.ival);
			getoken();
			break;
		case T_LNGCONST:
			if (!SkipExpression)
				pushlng(Ctx.Value.lval);
			getoken();
			break;
		case T_FLTCONST:
			if (!SkipExpression)
				pushflt(Ctx.Value.fval);
			getoken();
			break;
		case T_STRCONST:
			/*
		 	 * A string constant; similar to an array.
		 	 */
			if (!SkipExpression)
				push(0, 0, 1, RVALUE, sizeof(char), CHAR, NULL, &Ctx.Value, 1);
			getoken();
			break;
		case T_FUNCTREF:
		case T_FUNCTION:
		{
			if (Ctx.Curfunction == NULL)
				error(DECLARERR);
			if (!SkipExpression)
				push(FUNCT, 0, 1, RVALUE, 0, Ctx.Curfunction->type, NULL,
					(DATUM*)&Ctx.Curfunction, 1);
			getoken();
			return NULL;
		}
		case T_SYMBOL:		/* --- supports debugger --- */
		case T_IDENTIFIER:
			/*
		 	 * variable identifier
		 	 */
			if (SkipExpression)	{
				getoken();
				break;
			}
			if ((pvar = Ctx.Curvar) != 0)	{
				int isLvalue;

				if (Ctx.Curvar->vtype == ENUM)	{
					pushint(Ctx.Curvar->enumval);
					getoken();
					break;
				}
				getoken();
				/*
			 	 * it's a variable reference. The way a
			 	 * variable is represented on the stack
			 	 * depends on its type:
			 	 *
			 	 *            Ctx.Stackptr->
			 	 *            lvalue class value.iptr
			 	 * plain:       1      0   address of var
			 	 * pointers:    1      1   ptr to address of ptr
			 	 * arrays:      0      1   address of var
			 	 */

				Ctx.Value.cptr = DataAddress(pvar);

				/* arrays, functions, and addresses aren't lvalues */
				isLvalue = (!(isArray(pvar) ||
								((pvar->vkind & FUNCT) &&
									!isAddressOrPointer(pvar))));

				push(pvar->vkind, pvar->isunsigned, pvar->vclass,
					isLvalue, pvar->vsize, pvar->vtype,
					(VARIABLELIST*)&pvar->velem, &Ctx.Value, pvar->vconst);
				Ctx.Stackptr->vstruct = pvar->vstruct;
				Ctx.Stackptr->vqualifier = pvar->vqualifier;
				for (i = 0; i < MAXDIM; i++)
					Ctx.Stackptr->dims[i] = pvar->vdims[i];
			}
			else
				/*
			 	 * Undeclared symbol
			 	 */
				error(DECLARERR);
			break;
		case T_RPAREN:
			break;
		case T_EOF:
		case T_SEMICOLON:
				break;
		default:
			error(EXPRERR);
	}
	if ((pvar->vkind & STRUCTELEM) == 0)
		postop();
	return pvar;
}

static void postop()
{
	if (Ctx.Token==T_INCR || Ctx.Token==T_DECR)	{
		prepostop(1, Ctx.Token);
		getoken();
	}
}

static void prepostop(int ispost, char tok)
{
	/*
	 * This function parses the following right-to-left
	 * associative operators:
	 *
	 *    ++ --   pre and post increment and decrement
	 */
	char *p;
	DATUM v;
	char sign = tok == T_INCR ? 1 : -1;

	if (SkipExpression)
		return;
	if (ConstExpression)
		error(CONSTEXPRERR);
	if (!readonly(Ctx.Stackptr))	{
		if (StackItemisAddressOrPointer())	{
			if (Ctx.Stackptr->type == VOID)
				error(VOIDPTRERR);
			/*
			 * It's a pointer - save its old
			 * value then increment/decrement
			 * the pointer. This makes the item
			 * on top of the stack look like an
			 * array, which means it can no longer
			 * be used as an LVALUE. This doesn't
			 * really hurt, since it doesn't make
			 * much sense to say:
			 *   char *cp;
			 *   cp++ = value;
			 */
			p = *Ctx.Stackptr->value.pptr;

			*Ctx.Stackptr->value.pptr += sign * ElementWidth(Ctx.Stackptr);

			if (ispost)	{
				Ctx.Stackptr->value.pptr = (char **)p;
				Ctx.Stackptr->lvalue = 0;
			}
		}
		else	{
			/*
			 * It's a simple variable - save its
			 * old value then increment/decrement
			 * the variable. This makes the item
			 * on top of the stack look like a
			 * constant, which means it can no
			 * longer be used as an LVALUE.
			 */
			if (Ctx.Stackptr->type == CHAR)	{
				v.ival = *Ctx.Stackptr->value.cptr;
				*Ctx.Stackptr->value.cptr += sign;
				if (ispost)
					Ctx.Stackptr->value.cval = v.ival;
			}
			else if (Ctx.Stackptr->type == INT)	{
				v.ival = *Ctx.Stackptr->value.iptr;
				*Ctx.Stackptr->value.iptr += sign;
				if (ispost)
					Ctx.Stackptr->value.ival = v.ival;
			}
			else if (Ctx.Stackptr->type == LONG)	{
				v.lval = *Ctx.Stackptr->value.lptr;
				*Ctx.Stackptr->value.lptr += sign;
				if (ispost)
					Ctx.Stackptr->value.lval = v.lval;
			}
			else if (Ctx.Stackptr->type == FLOAT)	{
				v.fval = *Ctx.Stackptr->value.fptr;
				*Ctx.Stackptr->value.fptr += sign;
				if (ispost)
					Ctx.Stackptr->value.fval = v.fval;
			}
			else
				error(LVALERR);
			if (ispost)
				Ctx.Stackptr->lvalue = 0;
		}
	}
	else
		error(LVALERR);
}

