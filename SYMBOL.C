/*
 * QUINCY Interpreter - symbol table routines
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mem.h>
#include "qnc.h"

static void strucdef(char, VARIABLE *);
static VARIABLE *varlist(char, char, char, char, char, VARIABLELIST *, 
							VARIABLE *, VARIABLE *, int, int, int, int);
static void initexpr(VARIABLE *);
static VARIABLE *Declarator(char, VARIABLELIST*, int, VARIABLE*);
int ConstExpression;

FUNCTION *FindFunction(int fsymbolid)
{
	FUNCTION *FirstFunction = FunctionMemory;
	while (FirstFunction != NULL && FirstFunction < NextFunction)	{
		if (fsymbolid == FirstFunction->symbol)
			return FirstFunction;
		FirstFunction++;
	}
	return NULL;
}

void InstallFunction(FUNCTION *funcp)
{
	if (NextFunction == FunctionMemory+qCfg.MaxFunctions)
		error(TOOMANYFUNCERR);
	*NextFunction++ = *funcp;
}

static VARIABLE *FindVariable(int symbolid, VARIABLELIST *vartab,
						int BlkNesting, VARIABLE *Stopper, int isStruct)
{
	/*
	 * Search for a symbol name in the given variable
	 * linked list (variable table "vartab").
	 */
	VARIABLE *tvar = vartab->vlast;
	while (tvar != NULL)	{
		if (tvar == Stopper)	{
			tvar = NULL;
			break;
		}
		if (BlkNesting >= tvar->vBlkNesting)
			if (symbolid == tvar->vsymbolid)
				if (isStruct == (tvar->velem.vfirst != NULL))
					break;
		tvar = tvar->vprev;
	}
	return tvar;
}

VARIABLE *SearchVariable(int symbolid, int isStruct)
{
	/*
	 * Search:
	 * 1) current structure
	 * 2) current locals
	 * 3) current globals
	 * 4) library globals
	 * for a symbol name
	 */
	VARIABLE *pvar;

	if ((pvar = FindVariable(symbolid, &Ctx.Curstruct, 999, NULL, isStruct)) != NULL)
		return pvar;
	if (Ctx.Linkfunction)	{
		if ((pvar = FindVariable(symbolid,&Ctx.Linkfunction->locals,
				Ctx.Linkfunction->BlkNesting, NULL, isStruct)) != NULL)
			return pvar;
	}
	/* ----- this supports the debugger at runtime ------ */
	if (Ctx.Curfunc)	{
		if ((pvar = FindVariable(symbolid,
				&Ctx.Curfunc->fvar->locals,
				Ctx.Curfunc->BlkNesting, NULL, isStruct)) != NULL)
			return pvar;
	}
	if ((pvar = FindVariable(symbolid, &Globals, 0, NULL, isStruct)) != NULL)
		return pvar;
	return NULL;
}

VARIABLE *InstallVariable(VARIABLE *pvar,		/* the variable to be installed */
				          VARIABLELIST *pvartab,/* pointer to linked list of variables (a var table) */
				          int alloc,			/* allocate storage and initialize to zero if set */
						  int isArgument,
						  int BlkNesting,
						  int isStructdef)
{
	/*
	 * Install a new variable in the variable table
	 */
	VARIABLE *Nextvar = AllocVariable();
	VARIABLE *Thisvar;

	Assert(pvar != NULL);
	Assert(pvartab != NULL);

	*Nextvar = *pvar;
	Nextvar->vwidth = VariableWidth(Nextvar);
	Nextvar->vBlkNesting = BlkNesting;

	if (Nextvar->vqualifier & EXTERN)	{
		pvartab = &Globals;
		Nextvar->vBlkNesting = 0;
		alloc = 1;
	}

	/*
	 * Check for redeclaration
	 */
	/* --- search the table for the variable --- */
	if (pvar->vsymbolid)	{
		VARIABLE *existVar =
			FindVariable(pvar->vsymbolid, pvartab, 999, Blkvar, isStructdef);
		if (existVar != NULL)	{
			/* --- there's one with the same name --- */
			NullVariable(Nextvar);
			--Nextvar;
			if (pvar->vtype == existVar->vtype)	{
				if ((pvar->vqualifier) & EXTERN)
					/* --- extern declaration of declared variable --- */
					return existVar;
				if ((existVar->vqualifier) & EXTERN)	{
					existVar->vqualifier &= ~EXTERN;
					return existVar;
				}
			}
			error(MDEFERR);
		}
	}

	/* --- not there, add the variable to the table --- */

	if (alloc)	{
		/*
		 * Reserve data space
		 */
		char *ds = GetDataSpace(Nextvar->vwidth, 1);
		Nextvar->voffset = ds - DataSpace;
	}

	Nextvar->vnext = NULL;
	Nextvar->vprev = pvartab->vlast;
	if (pvartab->vlast != NULL)
		pvartab->vlast->vnext = Nextvar;
	pvartab->vlast = Nextvar;
	if (pvartab->vfirst == NULL)
		pvartab->vfirst = Nextvar;

	if ((Nextvar->vkind & LABEL) == 0 && !isTypedef(Nextvar))	{

		Nextvar->islocal = isArgument ? 2 : (pvartab != &Globals);

		if (!alloc && Nextvar->vtype != ENUM)	{

			/* --- compute offset of variable's data space --- */

			Thisvar = pvartab->vfirst;
			while (Thisvar != Nextvar)	{
				if (!isTypedef(Thisvar))
					if (Thisvar->vtype != ENUM)
						if (Thisvar->islocal == Nextvar->islocal)
							if (!Nextvar->vstatic)
								Nextvar->voffset += Thisvar->vwidth;
				Thisvar = Thisvar->vnext;
				Assert(Thisvar != NULL);
			}
		}
	}
	Nextvar->fileno = Ctx.CurrFileno;
	Nextvar->lineno = Ctx.CurrLineno;
	return Nextvar;
}

int ArrayElements(VARIABLE *pvar)
{
	int i, j = 1;
	for (i = 0; i < MAXDIM; i++)
		if (pvar->vdims[i])
			j *= pvar->vdims[i];
	return j;
}

int ArrayDimensions(VARIABLE *pvar)
{
	int i, j = 0;

	for (i = 0; i < MAXDIM; i++)
		if (pvar->vdims[i])
			j++;
	return j;
}

int VariableWidth(VARIABLE *pvar)
{
	int nbytes = pvar->vsize;
	if (isPointerArray(pvar))
		nbytes = ArrayElements(pvar) * sizeof(void *);
	else if (isArray(pvar))
		nbytes = ArrayElements(pvar) * pvar->vsize;
	else if (isPointer(pvar))
		nbytes = sizeof(void *);
	return nbytes;
}

void TypeQualifier(VARIABLE *pvar)
{
	int done = 0;
	while (!done)	{
		switch (Ctx.Token)	{
			case T_STATIC:
				if (pvar->vqualifier & (AUTO | EXTERN | REGISTER))
					error(DECLERR);
				pvar->vstatic = 1;
				break;
			case T_AUTO:
				if (pvar->vstatic || (pvar->vqualifier & (EXTERN | REGISTER)))
					error(DECLERR);
				pvar->vqualifier |= AUTO;
				break;
			case T_REGISTER:
				if (pvar->vstatic || (pvar->vqualifier & (EXTERN | AUTO)))
					error(DECLERR);
				pvar->vqualifier |= REGISTER;
				break;
			case T_CONST:
				if (pvar->vqualifier & VOLATILE)
					error(DECLERR);
				pvar->vconst = 1;
				break;
			case T_EXTERN:
				if (pvar->vstatic || (pvar->vqualifier & (AUTO | REGISTER)))
					error(DECLERR);
				pvar->vqualifier |= EXTERN;
				break;
			case T_VOLATILE:
				if (pvar->vconst)
					error(DECLERR);
				pvar->vqualifier |= VOLATILE;
				break;
			default:
				done = 1;
				break;
		}
		if (!done)
			getoken();
	}
}

static VARIABLE *DefineEnum(VARIABLELIST *pvartab, VARIABLE var,
					int isArgument, int BlkNesting, int StopComma,
					unsigned char **svp)
{
	VARIABLE *psvar = NULL;
	/*
	 * enum definition
	 */
	char enuminit[6];
	int einit = 0;

	if (Ctx.Token != T_LBRACE)
		error(ENUMERR);
	while (getoken() == T_SYMBOL)	{
		var.vsymbolid = Ctx.Value.ival;
		var.vtype = ENUM;
		var.vsize = sizeof(int);
		var.vconst = 1;
		psvar = InstallVariable(&var, pvartab, 0, isArgument, BlkNesting, 0);
		if (getoken() == T_ASSIGN)	{
			/* -------- enum initializer -------- */
			getoken();
			if (Ctx.Token == T_INTCONST)
				einit = Ctx.Value.ival;
			else 
				error(ENUMERR);
			getoken();
		}
		psvar->enumval = einit++;
		if (Ctx.Token != T_COMMA)
			break;
	}
	if (Ctx.Token != T_RBRACE)
		error(ENUMERR);
	*svp = Ctx.Progptr;
	getoken();
	return psvar;
}

static VARIABLE *DeclareEnum(VARIABLELIST *pvartab, VARIABLE var,
					int isArgument, int BlkNesting, int StopComma)
{
	VARIABLE *evar = NULL;
	unsigned char *svp;
	getoken();
	if (!isSymbol())
		error(ENUMERR);
	svp = Ctx.Progptr;
	getoken();
	if (Ctx.Token != T_SYMBOL)
		evar = DefineEnum(pvartab, var, isArgument, BlkNesting, StopComma, &svp);
	if (Ctx.Token == T_SYMBOL)	{
		/* ----- enum declaration becomes int ------- */
		Ctx.Progptr = svp;
		Ctx.Token = T_INT;
		evar = NULL;
	}
	return evar;
}

static VARIABLE *DeclareStructure(VARIABLELIST *pvartab, VARIABLE var,
					int isMember, int isArgument, int BlkNesting, int StopComma)
{
	/*
	 * Prepare a local variable entry
	 */
	VARIABLE *psvar;
	char tokn = Ctx.Token;

	var.vtype = MakeType(tokn);

	getoken();
	if (isSymbol())	{
		/*
		 * Found a "struct tag"
		 */
		if ((psvar = Ctx.Curvar) != NULL)	{
			getoken();
			if (Ctx.Token == T_CONST || Ctx.Token == T_VOLATILE)	{
				var.vconst = (Ctx.Token == T_CONST);
				getoken();
			}
			/*
			 * tag name already exists
			 */
			if (!psvar->velem.vfirst)
				/*
				 * but it's not a struct tag
				 */
				error(STRUCERR);
			else if (Ctx.Token == T_LBRACE)
				/*
				 * trying to redefine the struct tag
				 */
				error(MDEFERR);
			else	{
				/*
				 * expect a list of variables of
				 * type "struct tag"
				 */
				var.vsize = psvar->vsize;
				var.vtype = psvar->vtype;
				var.velem = psvar->velem;
				var.vstruct = psvar;
				psvar = varlist(tokn, isTypedef(&var),
					var.vconst,	var.vstatic, var.vqualifier,
					pvartab, NULL, &var,
					isMember, isArgument, BlkNesting, StopComma);
			}
		}
		else if (pvartab)	{
			/*
			 * Struct tag doesn't exist - expect
			 * its definition, then install it
			 * in the given symbol table.
			 */
			var.vsymbolid = Ctx.Value.ival;
			getoken();
			psvar = InstallVariable(&var, pvartab, 0, isArgument, BlkNesting, 1);
			psvar->vstruct = psvar;
			if (Ctx.Token != T_LBRACE)
				error(STRUCERR);
			strucdef(tokn, psvar);
			varlist(tokn, isTypedef(psvar), 0, 0, 0,
						pvartab, NULL, psvar,
				isMember, isArgument, BlkNesting, StopComma);
		}
		else
			/*
			 * Don't know what the struct tag is supposed
			 * to look like.
			 */
			error(DECLARERR);
	}
	else if (Ctx.Token==T_LBRACE)	{
		/*
		 * Found a "struct {", expect its definition
		 */
		if (isArgument)
			error(SYNTAXERR);
		var.vsymbolid = 0;	/* it has no id yet */
		strucdef(tokn, &var);
		psvar = varlist(tokn, isTypedef(&var), 0, 0, 0,
							pvartab, NULL, &var,
							isMember, 0, BlkNesting, StopComma);
		psvar->vstruct = psvar;
	}
	else
		error(LBRACERR);
	return psvar;
}

static VARIABLE *DeclareTypedef(VARIABLELIST *pvartab, VARIABLE var,
					int isMember, int isArgument, int BlkNesting, int StopComma)
{
	VARIABLE tvar;
	if (Ctx.Curvar == NULL || !isTypedef(Ctx.Curvar))
		error(DECLERR);
	/* --- typedef, provide implicit declarator --- */
	tvar = *Ctx.Curvar;
	tvar.vsymbolid = 0;
	tvar.vkind &= ~TYPEDEF;
	tvar.vstatic |= var.vstatic;
	tvar.vconst |= var.vconst;
	getoken();
	TypeQualifier(&tvar);
	return varlist(0, 0, tvar.vconst,
				tvar.vstatic, tvar.vqualifier, pvartab, &tvar, 0,
				isMember, isArgument, BlkNesting, StopComma);
}

static VARIABLE *DeclareNative(VARIABLELIST *pvartab, VARIABLE var,
					int isMember, int isArgument, int BlkNesting, int StopComma)
{
	/*
	 * native type declaration (int, char, etc.)
	 */
	char tokn = Ctx.Token;
	getoken();
	TypeQualifier(&var);
	return varlist(tokn, isTypedef(&var), var.vconst,
				var.vstatic, var.vqualifier, pvartab, NULL, NULL,
				isMember, isArgument, BlkNesting, StopComma);
}

VARIABLE *DeclareVariable(VARIABLELIST *pvartab,
					int isMember, int isArgument, int BlkNesting, int StopComma)
{
	/*
	 * Parse a declaration statement and add variables to the
	 * variable table pointed to by pvartab.
	 */
	VARIABLE var;

	NullVariable(&var);
	if (Ctx.Token == T_TYPEDEF)	{
		if (isArgument)
			error(TYPEDEFERR);
		var.vkind = TYPEDEF;
		getoken();
	}

	TypeQualifier(&var);

	if (Ctx.Token == T_ENUM)	{
		VARIABLE *evar = DeclareEnum(pvartab, var, isArgument, BlkNesting, StopComma);
		if (evar)
			return evar;
	}
	if (Ctx.Token == T_STRUCT || Ctx.Token == T_UNION)
		return DeclareStructure(pvartab, var, isMember, isArgument, BlkNesting, StopComma);
	if (isSymbol())
		return DeclareTypedef(pvartab, var, isMember, isArgument, BlkNesting, StopComma);
	return DeclareNative(pvartab, var, isMember, isArgument, BlkNesting, StopComma);
}

static void strucdef(char tokn, VARIABLE *psvar)
{
	/*
	 * Parse a struct definition.
	 */
	unsigned wid;
	VARIABLE *pvar;

	/*
	 * Get rid of the "{" and any blank lines
	 */
	getoken();

	while(istypespec() ||
			Ctx.Token == T_CONST ||
			Ctx.Token == T_VOLATILE ||
			Ctx.Token == T_VOID)	{
		pvar = DeclareVariable((VARIABLELIST*)&psvar->velem, 1, 0, psvar->vBlkNesting, 0);
		stmtend();
	}

	if (Ctx.Token == T_RBRACE)
		getoken();
	else
		error(RBRACERR);
	/*
	 * Now change all of the variables declared in the
	 * structure or union to structure element types,
	 * build their data offsets, and compute the structure
	 * width.
	 */
	pvar = psvar->velem.vfirst;
	while (pvar != NULL)	{
		pvar->vkind |= STRUCTELEM;
		wid = VariableWidth(pvar);
		if (tokn == T_UNION)	{
			pvar->voffset = 0;
			psvar->vsize = max(psvar->vsize, wid);
		}
		else	{
			pvar->voffset = psvar->vsize;
			psvar->vsize += wid;
		}
		pvar = pvar->vnext;
	}
}

/* --- start from the first type token and resolve the
   declaration's type --- */
char MakeTypeToken(char tokn, int *sawunsigned)
{
	int done = 0;
	if (sawunsigned)
		*sawunsigned = tokn == T_UNSIGNED;
	while (!done)	{
		switch (Ctx.Token)	{
			case T_DOUBLE:
			case T_LONG:
			case T_CHAR:
				tokn = Ctx.Token;
				getoken();
				break;
			case T_INT:
				if (tokn != T_LONG)
					tokn = T_INT;
				getoken();
				break;
			case T_UNSIGNED:
				if (sawunsigned)
					*sawunsigned = 1;
			case T_SHORT:
				tokn = T_INT;
				getoken();
				break;
			default:
				done = 1;
				break;
		}
	}
	if (tokn == T_UNSIGNED)
		tokn = T_INT;
	return tokn;
}

static VARIABLE *varlist(char tokn, char VarisTypedef,
						char vconst, char vstatic, char vqualifier,
						VARIABLELIST *pvartab, VARIABLE *Typedef,
						VARIABLE *psvar,
						int isMember, int isArgument, int BlkNesting,
						int StopComma)
{
	/*
	 * Parse a declarator list
	 *
	 * declarator-list:
	 *    declarator
	 *    declarator = initializer
	 *    declarator-list , declarator
	 *
	 * The argument "tokn" is the Ctx.Token form of the declaration-specifier
	 * "char", "int", "struct", "union", etc.
	 */
	VARIABLE *pvar, *svar;
	int sawunsigned;
	tokn = MakeTypeToken(tokn, &sawunsigned);

	while ((pvar = Declarator(tokn, pvartab, isArgument, Typedef)) != NULL)	{
		int alloc = 0;
		if (sawunsigned)
			pvar->isunsigned = 1;
		if (Typedef != NULL)	{
			/* --- variable is defined by a typedef --- */
			if (isArgument)	{
				/* function args don't have dimensions */
				int i;
				for (i = 0; i < MAXDIM; i++)
					pvar->vdims[i] = 0;
			}
			while (Ctx.Token == T_PTR)	{
				pvar->vclass++;
				getoken();
			}
			if (isSymbol())	{
				pvar->vsymbolid = Ctx.Value.ival;
				getoken();
			}
		}
		/* ---- if it's a pointer, vconst means what it points to ---- */
		if (pvar->vclass)
			vconst <<= 1;
		if (Typedef == NULL)	{
			if (VarisTypedef)
				pvar->vkind |= TYPEDEF;
			pvar->vconst |= vconst;
			pvar->vstatic = vstatic;
			pvar->vqualifier = vqualifier;
			if (tokn==T_STRUCT || tokn==T_UNION)	{
				/*
			 	* Structures and unions:  We need to copy
			 	* over the declarator's attributes to the
			 	* structure variable.
			 	*/
				pvar->vsize = psvar->vsize;
				pvar->vtype = psvar->vtype;
				pvar->velem = psvar->velem;
				pvar->vconst |= psvar->vconst;
				pvar->vstruct = psvar->vstruct;
				pvar->vqualifier = psvar->vqualifier;
			}
		}
		if (pvartab)	{
			/*
			 * It's a plain variable; install it.
			 */
			/* --- only allocate memory for globals and statics
				      when linking --- */
			alloc = (!isTypedef(pvar)    &&
					 pvar->vtype != ENUM &&
					 !isMember           &&
					 !isArgument         &&
					 (pvar->vstatic || pvartab == &Globals));
			pvar = InstallVariable(pvar, pvartab, alloc, isArgument, BlkNesting, 0);
		}

		if (Ctx.Token==T_ASSIGN)	{
			if (pvar->isinitialized)
				error(INITERR);
			if (pvartab != &Globals && (vqualifier & EXTERN))
				error(INITERR);
			pvar->isinitialized = 1;
			pvar->vqualifier &= ~EXTERN;
			if (isArgument || (pvar->vkind & TYPEDEF))
				error(INITERR);
			if (!Linking && !pvar->vstatic)
				stmtbegin();
			getoken();		/* get rid of '=' */
			if (alloc)	{
				int braceneeded = isArray(pvar) &&
						!(pvar->vtype == CHAR && Ctx.Token == T_STRCONST);
				/* ----- initializers ------ */
				if (braceneeded && Ctx.Token != T_LBRACE)
					error(LBRACERR);

				Initializer(pvar, DataAddress(pvar), 0);

				if (Ctx.Token == T_RBRACE)
					getoken();
				else if (braceneeded)
					error(RBRACERR);
			}
			else	{
				/* --- skip initializers for args, members,
					and when linking autos --- */
				if (Ctx.Token == T_LBRACE)
					skip(T_LBRACE, T_RBRACE);
				else	{
					while (Ctx.Token != T_COMMA && Ctx.Token != T_SEMICOLON)	{
						if (getoken() == T_EOF)
							error(INITERR);
						if (Ctx.Token == T_LPAREN)
							skip(T_LPAREN, T_RPAREN);
					}
				}
			}
		}

		if (Ctx.Token != T_COMMA || StopComma)
			break;
		getoken();
	}
	return pvar;
}

void Initializer(VARIABLE *pvar, char *baseaddr, int level)
{
	/*
	 * Parse variable initialization expressions
	 */
	union	{
		char *cp;
		int *ip;
		long *jp;
		double *fp;
		void **pp;
	} ptrs;

	VARIABLE *pevar;

	int i, len;

	if (level == 0 && isArray(pvar))	{
		/* --- initializing the elements of an array --- */
		int dims, elems;

		if (pvar->vtype == CHAR && Ctx.Token == T_STRCONST)	{
			/* ---- string literal initialization ---- */
			Initializer(pvar, baseaddr + i, 1);
			return;
		}

		dims = ArrayDimensions(pvar);
		elems = ArrayElements(pvar);

		if (Ctx.Token == T_LBRACE)
			getoken();

		for (i = 0; i < elems; i++)	{
			int ln;

			if (isPointer(pvar))
				ln = sizeof(void *);
			else
				ln = pvar->vsize;

			if (Ctx.Token == T_LBRACE && dims > 1)	{
				/* --- compute # elements in the inner array --- */
				VARIABLE avar;
				int dm;
				int wd = 1, j = pvar->vclass-1;

				if (j < 0)
					error(INITERR);

				while (j)
					wd *= pvar->vdims[j--];

				/* --- build a variable to represent the inner array --- */
				avar = *pvar;
				--avar.vclass;
				for (dm = 0; dm < MAXDIM-1; dm++)
					avar.vdims[dm] = avar.vdims[dm+1];
				avar.vdims[dm] = 0;

				/* ---- initialize the inner array ---- */
				Initializer(&avar, baseaddr + i * ln, 0);
				i += wd-1;

				if (Ctx.Token != T_RBRACE)
					error(RBRACERR);
				getoken();
			}
			else
				Initializer(pvar, baseaddr + i * ln, level+1);
			if (Ctx.Token == ',')	{
				getoken();
				/* --- test for too many initializers --- */
				if (i >= elems-1)
					error(TOOMANYINITERR);
			}
			else
				break;
		}
		return;
	}

	if ((pvar->vtype == STRUCT || pvar->vtype == UNION) && !isPointer(pvar))	{
		/* --- initializing a struct or union --- */
		if (Ctx.Token != T_LBRACE)	{
			/* --- initializing with another struct or union --- */
			/* --- push the receiving struct --- */
			push(pvar->vkind, pvar->isunsigned, pvar->vclass, 1,
					pvar->vsize, pvar->vtype,
					(VARIABLELIST*)&pvar->velem,
					(DATUM*)&baseaddr, pvar->vconst);
			initexpr(pvar);
			if (pvar->vstruct != Ctx.Stackptr->vstruct)
				error(INITERR);
			assignment();
			pop();
			return;
		}
		getoken();
		pevar = pvar->velem.vfirst;
		for (;;)	{
			pevar->vstatic = pvar->vstatic;
			Initializer(pevar, baseaddr+(int)pevar->voffset, level);
			pevar = pevar->vnext;
			if (Ctx.Token == ',')	{
				getoken();
				/* --- test for too many initializers --- */
				if (pevar == NULL)
					error(TOOMANYINITERR);
			}
			else
				break;
		}
		if (Ctx.Token != T_RBRACE)
			error(RBRACERR);
		getoken();
		return;
	}

	/*
	 * Get variable's data address
	 */
	ptrs.cp = baseaddr;

	initexpr(pvar);
	if (StackItemisString())	{
		/* ---- Item on stack is a string literal ---- */
		if (!(pvar->vtype == CHAR && pvar->vclass))
			/* -- can initialize only char *, char [] or char [n] with string -- */
			error(INITERR);
		if (!isPointerArray(pvar))	{
			if (isArray(pvar))	{
				char *cp = popptr();
				if (pvar->vdims[0] < strlen(cp))
				/* --- initializer cannot be longer than char array --- */
					error(INITERR);
				strcpy(ptrs.cp, cp);
				return;
			}
		}
	}
	if (isPointer(pvar))	{
		/* ---- initializing a pointer with an address ----- */
		*ptrs.pp = popptr();
		return;
	}
	/*
	 * Variable is a plain variable.
	 */
	if (StackItemisAddressOrPointer())
		error(INITERR);
	switch (pvar->vtype)	{
		case CHAR:
			*ptrs.cp = popint();
			break;
		case INT:
			*ptrs.ip = popint();
			break;
		case LONG:
			*ptrs.jp = poplng();
			break;
		case FLOAT:
			*ptrs.fp = popflt();
			break;
		default:
			pop();
			break;
	}
}

static void initexpr(VARIABLE *pvar)
{
	/*
	 * Parse an initialization expression
	 */
	ITEM *sp = Ctx.Stackptr;
	ConstExpression = (!pvar->islocal) ||
						pvar->vstatic  ||
						isArray(pvar)  ||
						(pvar->vkind & STRUCTELEM);
	cond();
	if (sp==Ctx.Stackptr)
		/*
		 * nothing was pushed (null expression)
		 */
		error(INITERR);
	ConstExpression = 0;
}

char MakeType(char tok)
{
	switch (tok)	{
		case T_CHAR:
			return CHAR;
		case T_SHORT:
		case T_UNSIGNED:
		case T_ENUM:
		case T_INT:
			return INT;
		case T_LONG:
			return LONG;
		case T_FLOAT:
		case T_DOUBLE:
			return FLOAT;
		case T_STRUCT:
			return STRUCT;
		case T_UNION:
			return UNION;
		case T_VOID:
			return VOID;
		default:
			break;
	}
	return tok;
}

int TypeSize(char type)
{
	char typesize[] = {
		0,
		sizeof(char),
		sizeof(int),
		sizeof(long),
		sizeof(double),
		0,
		0
	};
	return typesize[type];
}

static void SetType(VARIABLE *var, char tok)
{
	var->vtype = MakeType(tok);
	var->vsize = TypeSize(var->vtype);
}

static VARIABLE *Declarator(char tokn, VARIABLELIST *pvartab, int isArgument,
										VARIABLE *Typedef)
{
	/*
	 * Parse a declarator:
	 *
	 * declarator:
	 *    symbol
	 *    * declarator
	 *    declarator [ constant expression ]
	 */
	VARIABLE *pvar;
	VARIABLE var;
	static VARIABLE svar;
	int argc;

	svar.velem.vfirst = svar.velem.vlast = 0;
	if (Ctx.Token==T_LPAREN)	{
		getoken();
		if ((pvar = Declarator(tokn, pvartab, isArgument, Typedef)) != NULL)	{
			if (Ctx.Token==T_RPAREN)	{
				/*
				 * Check for function pointer declaration
				 */
				if (getoken() == T_LPAREN)	{
					pvar->vkind |= FUNCT;
					skip(T_LPAREN, T_RPAREN);
				}
			}
			else
				error(RPARENERR);
		}
	}
	else if (Ctx.Token==T_PTR)	{
		/*
		 * it's a pointer
		 */
		int isconst = 0;
		getoken();
		if (Ctx.Token == T_VOLATILE)
			getoken();
		else if ((isconst = (Ctx.Token == T_CONST)) != 0)
			getoken();
		if ((pvar = Declarator(tokn, pvartab, isArgument, Typedef)) != NULL)	{
			pvar->vconst |= isconst;
			++pvar->vclass;
		}
	}
	else if (isSymbol())	{
		/*
		 * Save the symbol's name and size and initialize
		 * to zero all other attributes.
		 */
		if (Typedef != NULL)
			var = *Typedef;
		else	{
			NullVariable(&var);
			SetType(&var, tokn);
		}
		var.vsymbolid = Ctx.Value.ival;
		getoken();
		/*
		 * compute its length
		 */
		while (Ctx.Token==T_LBRACKET)	{
			if (var.vclass++ == MAXDIM)
				error(MDIMERR);

			getoken();
			/*
			 * compute the dimension
			 */
			if (Ctx.Token==T_RBRACKET)	{
				/* arrayname[] (empty dimension, could be pointer) */
				getoken();
				if (pvartab != NULL && Ctx.Token == T_ASSIGN)	{
					unsigned char *svprogptr = Ctx.Progptr;
					getoken();
					/* --- parse ahead to compute the dimension --- */
					if (var.vtype == STRUCT || var.vtype == UNION)	{
						if (Ctx.Token != T_LBRACE)
							error(LBRACERR);
						var.vdims[var.vclass-1]++;
						while (Ctx.Token != T_SEMICOLON)	{
							if (Ctx.Token == T_COMMA)
								var.vdims[var.vclass-1]++;
							else if (Ctx.Token == T_EOF)
								error(INITERR);
							getoken();
							if (Ctx.Token == T_LBRACE)
								skip(T_LBRACE, T_RBRACE);
						}
					}
					else	{
						if (Ctx.Token == T_LBRACE)	{
							getoken();
							ConstExpression = 1;
							argc = expression();
							ConstExpression = 0;
							if (Ctx.Token != T_RBRACE)
								error(RBRACERR);
							var.vdims[var.vclass-1] = argc;
							popnint(argc);
						}
						else if (var.vtype == CHAR && Ctx.Token == T_STRCONST)	{
							char *cp;
							primary();
							cp = popptr();
							var.vdims[var.vclass-1] = strlen(cp) + 1;
						}
					}
					/* --- return to assignment position --- */
					Ctx.Progptr = svprogptr;
					Ctx.Token = T_ASSIGN;
				}
			}
			else	{
				ConstExpression = 1;
				argc = expression();
				ConstExpression = 0;

				if (Ctx.Token!=T_RBRACKET)
					error(RPARENERR);
				getoken();
				if ((var.vdims[var.vclass-1] = popnint(argc)) < 0)
					/*
					 * negative array size.
					 */
					error(SUBSERR);
				if (isArgument)
					/*
					 * This is a function argument, the inner dimension
					 * length is discarded.
					 */
					var.vdims[0] = 0;
			}
		}

		if (pvartab)	{
			/*
			 * We are going to be installing this variable
			 * in a symbol table somewhere, so...
			 * copy the auto to the static VARIABLE.
			 */
			svar = var;
			pvar = &svar;
		}
		else	{
			/*
			 * pvartab==NULL implies that this is a declaration
			 * of a formal argument and it was already
			 * previously installed in the function's local
			 * symbol table. Search for the variable in the
			 * current locals and change its attributes.
			 */
			if ((pvar = FindVariable(var.vsymbolid,
					&Ctx.Linkfunction->locals, 1, NULL, 0)) != NULL)	{
				int i;
				pvar->vclass = var.vclass;
				pvar->vtype = var.vtype;
				pvar->vsize = var.vsize;
				for (i = 0; i < MAXDIM; i++)
					pvar->vdims[i] = var.vdims[i];
				pvar->vconst = var.vconst;
			}
			else
				/*
				 * Declared variable not in formal
				 * argument list.
				 */
				error(DECLARERR);
		}
	}
	else	{
		/*
		 * This is a kludge to allow definition of struct tags:
		 * struct stype {
		 *	.
		 *	.
		 *	.
		 * } ;
		 * normally, this function expects to parse a declarator
		 * but instead in the above case it sees only the end
		 * of the declaration statement.  This kludge allows
		 * illegal declaration statements like:
		 *
		 *    char ;
		 *
		 * to sneak by undetected...
		 */
		if (pvartab)
			return NULL;
		/*
		 * If we're not installing this symbol in a symbol
		 * table, then it's either a function argument declaration
		 * or a typecast.  In either case, set up the static variable
		 * with as much info we have about it.
		 */
		if (Typedef != NULL)
			return Typedef;
		NullVariable(&svar);
		SetType(&svar, tokn);
		return &svar;
	}

	return pvar;
}

void *GetDataSpace(int sz, int init)
{
	char *ds;
	if (Ctx.NextData == DataSpace + qCfg.MaxDataSpace)
		error(DATASPACERR);
	ds = Ctx.NextData;
	if (sz)	{
		Ctx.NextData += sz;
		if (init)
			memset(ds, 0, sz);
	}
	return ds;
}

void *AllocVariable(void)
{
	if (Ctx.NextVar == VariableMemory+qCfg.MaxVariables)
		error(TOOMANYVARERR);
	return Ctx.NextVar++;
}

void *DataAddress(VARIABLE *pvar)
{
	char *vdata;
	if (pvar->vkind & STRUCTELEM)	{
		/*
		 * the only time DataAddress is asked for the
		 * address of a structure element is when the
		 * structure itself is on the stack and the
		 * program is dereferencing the member.
		 * get the structure object's address
		 * and add the element's offset.
		 */
		vdata = Ctx.Stackptr->value.cptr + pvar->voffset;

	}
	else if (pvar->islocal && !pvar->vstatic)	{
		Assert(Ctx.Curfunc != NULL);
		vdata = Ctx.Curfunc->ldata + pvar->voffset;
		if (pvar->islocal == 1)
			/* --- auto and not argument --- */
			vdata += Ctx.Curfunc->arglength;
	}
	else 
		vdata = DataSpace + pvar->voffset;
	return vdata;
}

