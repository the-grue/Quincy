/*
 * QUINCY Interpreter - declarations linker
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include "sys.h"
#include "qnc.h"

static void arglist(void);
static void LocalDeclarations(void);

static VARIABLE FuncVariable;
static unsigned char *errptr;
static int fconst;
static char protoreturn;
static char stopToken;
static int protoclass;

int istypespec()
{
	static char typespecs[] = {
		T_CHAR, T_INT, T_LONG, T_FLOAT, T_DOUBLE,
		T_STRUCT, T_UNION, T_ENUM, T_UNSIGNED, T_SHORT, 0
	};
	if (isSymbol())
		return (Ctx.Curvar && isTypedef(Ctx.Curvar));
	return strchr(typespecs, Ctx.Token) != NULL;
}

static int isParameterType(void)
{
	return (istypespec()   ||
			Ctx.Token == T_VOID     ||
			Ctx.Token == T_CONST    ||
			Ctx.Token == T_VOLATILE	||
			Ctx.Token == T_ELLIPSE);
}

static int isLocalType(void)
{
	return (istypespec()   ||
			Ctx.Token == T_STATIC   ||
			Ctx.Token == T_VOID     ||
			Ctx.Token == T_EXTERN   ||
			Ctx.Token == T_CONST    ||
			Ctx.Token == T_REGISTER ||
			Ctx.Token == T_AUTO     ||
			Ctx.Token == T_VOLATILE);
}

/* ----- add to a function's prototype ----- */
void AddPro(unsigned char pro)
{
	if (NextProto >= PrototypeMemory + qCfg.MaxPrototype)
		error(OMERR);
	*++NextProto = pro;
}


void linkerror(int errnum)
{
	Ctx.Progptr = errptr;
	error(errnum);
}

/* --- test for type declaration and, if so, scan up to identifier --- */
int isTypeDeclaration(void)
{
	int done = 0;
	int istypedecl = 0;

	Ctx.svpptr = Ctx.Progptr;
	Ctx.svToken = Ctx.Token;
	Ctx.svCurvar = Ctx.Curvar;
	errptr = Ctx.Progptr;

	while (!done)	{
		switch (Ctx.Token)	{
			case T_SYMBOL:
			case T_IDENTIFIER:
				if (Ctx.Curvar == NULL || !isTypedef(Ctx.Curvar))	{
					done = 1;
					break;
				}
			case T_CONST:
			case T_STATIC:
			case T_VOLATILE:
			case T_TYPEDEF:
			case T_VOID:
			case T_CHAR:
			case T_INT:
			case T_LONG:
			case T_FLOAT:
			case T_DOUBLE:
			case T_SHORT:
			case T_UNSIGNED:
				istypedecl = 1;
				getoken();
				break;
			case T_STRUCT:
			case T_UNION:
			case T_ENUM:
				istypedecl = 1;
				getoken();
				if (isSymbol())
					getoken();
				break;
			default:
				done = 1;
				break;
		}
	}
	if (istypedecl)
		while (Ctx.Token == T_PTR)
			getoken();
	return istypedecl;
}

static void TypeDeclaration(VARIABLELIST *vartab)
{
	VARIABLE *var;
	Ctx.Progptr = Ctx.svpptr;
	Ctx.Token = Ctx.svToken;
	Ctx.Curvar = Ctx.svCurvar;
	var = DeclareVariable(vartab, 0, 0, 0, 0);
	if (vartab == &Globals && (var->vqualifier & (REGISTER | AUTO)))
		error(DECLERR);
	stmtend();
	fconst = 0;
}

static void FunctionPrototype(void)
{
	int sawunsigned;
	char tokn;

	Ctx.Progptr = Ctx.svpptr;
	Ctx.Token = Ctx.svToken;
	Ctx.Curvar = Ctx.svCurvar;

	NullVariable(&FuncVariable);
	FuncVariable.vtype = INT;
	FuncVariable.vsize = sizeof(int);

	TypeQualifier(&FuncVariable);
	tokn = Ctx.Token;
	if (Ctx.Token != T_FUNCTION && Ctx.Token != T_FUNCTREF)
		getoken();
	tokn = MakeTypeToken(tokn, &sawunsigned);

	/* ---- establish prototyped return value ------ */
	protoreturn = MakeType(tokn);
	if (tokn == T_STRUCT || tokn == T_UNION || tokn == T_ENUM)	{
		if (Ctx.Token != T_SYMBOL)
			linkerror(NOIDENTERR);
		getoken();
	}
	TypeQualifier(&FuncVariable);
	/* --- establish return type indirection --- */
	protoclass = 0;
	while (Ctx.Token == T_PTR)	{
		protoclass++;		/* return type indirection */
		getoken();
	}
	if (Ctx.Token != T_FUNCTION && Ctx.Token != T_FUNCTREF)
		linkerror(NOIDENTERR);
}
static void AddStructProto(void)
{
	*(VARIABLE**)(NextProto+1) = Ctx.Curvar->vstruct;
	NextProto += sizeof(VARIABLE **);
	if (NextProto > PrototypeMemory + qCfg.MaxPrototype)
		error(OMERR);
}
static void BuildPrototype(void)
{
	/* -------- build the parameter prototype ------- */
	char indir = 0;
	char donetype = 0;

	while (Ctx.Token != stopToken && Ctx.Token != T_EOF)	{
		if (istypespec() || Ctx.Token == T_ELLIPSE || Ctx.Token == T_VOID)	{
			if (donetype)	{
				AddPro(indir);
				indir = 0;
			}
			donetype = 1;
			if (isSymbol())	{
				/* --- argument is a typedef --- */
				AddPro(Ctx.Curvar->vtype);
				if (Ctx.Curvar->vtype == STRUCT || Ctx.Curvar->vtype == UNION)
					AddStructProto();
				indir = Ctx.Curvar->vclass;
			}
			else	{
				char tokn = MakeTypeToken(Ctx.Token, NULL);
				AddPro(MakeType(tokn));
				if (tokn == T_STRUCT || tokn == T_UNION)	{
					getoken();
					AddStructProto();
				}
				if (Ctx.Token == stopToken)
					break;
			}
		}
		if (Ctx.Token == T_PTR || Ctx.Token == T_LBRACKET)
			indir++;
		getoken();
	}
	AddPro(indir);

	if (Ctx.Token == T_EOF)
		linkerror(LBRACERR);
}

static void TestPrototype(unsigned char *pros, int type, int fclass)
{
	unsigned char *pr1, *pr2;
	pr1 = pros;
	pr2 = Ctx.Linkfunction->proto;
	while (*pr1 != 0xff)	{
		if (*pr1 != *pr2)
			linkerror(REDEFERR);
		pr1++, pr2++;
		if (*(pr1-1) == STRUCT || *(pr1-1) == UNION)	{
			if (*(VARIABLE**)pr1 != *(VARIABLE**)pr2)
				linkerror(REDEFERR);
			pr1 += sizeof(VARIABLE**);
			pr2 += sizeof(VARIABLE**);
		}
	}
	/* ---- make sure they both have the same number of arguments --- */
	if (*pr2 != 0xff)
		linkerror(REDEFERR);

	/* ----- make sure the return types match ---- */
	if (Ctx.Linkfunction->type != type || protoclass != fclass)
		linkerror(REDEFERR);

}

static void FunctionDeclaration(VARIABLELIST *vartab)
{
	/*
	 * function declaration.
	 */
	unsigned char *pros;
	int type, fclass;
	int lineno;
	int fileno;
	VARIABLE *Funcvar;
	VARIABLE *svNextvar;


	while (Ctx.Token == T_FUNCTREF || Ctx.Token == T_FUNCTION)	{
		Assert(Ctx.Curfunction != NULL);
		Ctx.Linkfunction = Ctx.Curfunction;
		lineno = Ctx.CurrLineno;
		fileno = Ctx.CurrFileno;
		errptr = Ctx.Progptr;
		type = Ctx.Linkfunction->type;
		fclass = Ctx.Linkfunction->class;
		Ctx.Linkfunction->type = protoreturn;
		Ctx.Linkfunction->fconst = fconst;

		if (getoken() != T_LPAREN)
			linkerror(LPARENERR);
		/* ----- declare local variables for function arguments --- */
		svNextvar = Ctx.NextVar;
		arglist();

		getoken();
		if (isParameterType() || Ctx.Token == T_PTR)
			stopToken = T_RPAREN;	/* ANSI declarator */
		else 
			stopToken = T_LBRACE;	/* K&R declarator */

		pros = NextProto+1;
		if (Ctx.Token != T_RPAREN)
			BuildPrototype();
		else	{
			/* ---- func() insert void type and 0 indirection ---- */
			AddPro(VOID);
			AddPro(0);
		}
		AddPro(0xff);

		if (Ctx.Token == T_LBRACE)
			/* --- the prototype scan stopped at '{' --- */
			break;
		/* --- the prototype argument scan stopped at ')' --- */
		getoken();
		if (Ctx.Token != T_SEMICOLON && Ctx.Token != T_COMMA)
			break;

		/* --- function prototype --- */
		Ctx.svpptr = Ctx.Progptr;
		/* --- delete argument declarations --- */
		Ctx.Linkfunction->locals.vfirst = NULL;
		Ctx.Linkfunction->locals.vlast = NULL;
		Ctx.NextVar = svNextvar;
		Assert(Ctx.NextVar != NULL);
		if (Ctx.NextVar->vprev)
			Ctx.NextVar->vprev->vnext = NULL;
		Ctx.Linkfunction->proto = pros;
		Ctx.Linkfunction->class = protoclass;
		if (Ctx.Token == T_SEMICOLON)
			break;
		getoken();
		if (Ctx.Token != T_FUNCTREF && Ctx.Token != T_FUNCTION)
			error(FUNCNAMERR);
	}
	if (Ctx.Token == T_SEMICOLON)
		getoken();
	else	{
		/* ---- function declaration ---- */
		if (vartab != &Globals)
			error(RBRACERR);
		while (Ctx.Token != T_LBRACE && Ctx.Token != T_EOF)
			getoken();
		if (Ctx.Token != T_LBRACE)
			linkerror(LBRACERR);

		/* --- installed function must be a prototype --- */
		if (Ctx.Linkfunction->code != NULL)
			linkerror(MDEFERR);
		Ctx.Linkfunction->code = Ctx.Progptr-1;
		Ctx.Linkfunction->fileno = fileno;
		Ctx.Linkfunction->lineno = lineno;
		if (Ctx.Linkfunction->proto != NULL)	{
			TestPrototype(pros, type, fclass);
			/* ---- don't need this prototype any more ---- */
			NextProto = pros-1;
		}
		/* ----- local declaraions ----- */
		Assert(Ctx.Token == T_LBRACE);
		LocalDeclarations();

		if (Ctx.Linkfunction->proto == NULL)
			Ctx.Linkfunction->proto = pros;
		Ctx.Linkfunction->class = protoclass;

		Funcvar = Ctx.Linkfunction->locals.vfirst;
		while (Funcvar != NULL)	{
			if (!Funcvar->vstatic && Funcvar->islocal == 1)
				Ctx.Linkfunction->width += Funcvar->vwidth;
			Funcvar = Funcvar->vnext;
		}
	}
	protoclass = 0;
	protoreturn = INT;
	Ctx.Linkfunction = NULL;
}
static void CheckDeclarations(void)
{
	VARIABLE *var = Globals.vfirst;
	FUNCTION *Function = FunctionMemory;
	while (var != NULL)	{
		if (var->vqualifier & EXTERN)	{
			Ctx.CurrFileno = var->fileno;
			Ctx.CurrLineno = var->lineno;
			error(UNRESOLVEDERR);
		}
		var = var->vnext;
	}
	while (Function < NextFunction)	{
		int braces = 0;
		if (Function->libcode == 0 && Function->code == NULL)	{
			Ctx.CurrFileno = Function->fileno;
			Ctx.CurrLineno = Function->lineno;
			error(UNDEFUNCERR);
		}
		Ctx.Progptr = Function->code;
		do	{
			switch (getoken())	{
				case T_LBRACE:
					braces++;
					break;
				case T_RBRACE:
					--braces;
					break;
				case T_ENUM:
					getoken();
					break;
				case T_SYMBOL:
					getoken();
					error(Ctx.Token == T_LPAREN ? NOFUNCERR : DECLARERR);
				default:
					break;
			}
		} while (braces != 0);
		Function++;
	}
}
void link(VARIABLELIST *vartab)
{
	/*
	 * Allocate global variables
	 */
	protoclass = 0;
	protoreturn = INT;
	fconst = 0;
	Linking = 1;
	getoken();
	while (Ctx.Token!=T_EOF)	{
		handshake();   /* to keep D-Flat clock ticking */
		isTypeDeclaration();
		/*
		 * type declaration, function declaration, or prototype
		 */
		/* (immediate '(' is function pointer, which is a type) */
		if (Ctx.Token == T_LPAREN || getoken() != T_LPAREN)	{
			if (Ctx.Token != T_FUNCTREF && Ctx.Token != T_FUNCTION)	{
				/* ----- data type ------- */
				TypeDeclaration(vartab);
				continue;
			}
		}
		FunctionPrototype();
		if (Ctx.Token == T_FUNCTREF || Ctx.Token == T_FUNCTION)
			FunctionDeclaration(vartab);
	}
	CheckDeclarations();
	Linking = 0;
}

void ConvertIdentifier(void)
{
	/* --- convert T_SYMBOL to T_IDENTIFIER --- */
	if (Ctx.Curvar != NULL)	{
		*(Ctx.Progptr-(1+sizeof(int))) = T_IDENTIFIER;
		*(unsigned*)(Ctx.Progptr-sizeof(int)) = FP_OFF(Ctx.Curvar);
	}
}

static void InnerDeclarations(int inStruct)
{
	VARIABLELIST *cstruct = NULL;
	VARIABLELIST svcs;
	while (Ctx.Token != T_RBRACE && Ctx.Token != T_EOF)	{
		if (Ctx.Token == T_ARROW || Ctx.Token == T_DOT)	{
			svcs = Ctx.Curstruct;
			Ctx.Curstruct = *cstruct;
			while (getoken() != T_SYMBOL)
				if (Ctx.Token == T_EOF)
					break;
			if (Ctx.Token != T_SYMBOL)
				error(ELEMERR);
			cstruct = NULL;
			Ctx.Curstruct = svcs;
		}
		if (Ctx.Token == T_STRUCT || Ctx.Token == T_UNION)	{
			getoken();
			svcs = Ctx.Curstruct;
			Ctx.Curstruct = *(VARIABLELIST*)&Ctx.Curvar->velem;
			while (Ctx.Token == T_SYMBOL)	{
				ConvertIdentifier();
				getoken();
			}
			if (Ctx.Token == T_LBRACE)	{
				getoken();
				InnerDeclarations(1);
			}
			Ctx.Curstruct = svcs;
		}
		if (Ctx.Token == T_LBRACE)	{
			if (!inStruct)
				LocalDeclarations();
			continue;
		}
		if (Ctx.Token == T_SYMBOL)	{
			ConvertIdentifier();
			if (Ctx.Curvar && Ctx.Curvar->velem.vfirst)
				cstruct = (VARIABLELIST*)&(Ctx.Curvar->velem);
		}
		getoken();
	}
	if (Ctx.Token != T_RBRACE)
		error(RBRACERR);
}

static void LocalDeclarations(void)
{
	VARIABLE *svBlkvar = Blkvar;
	unsigned char *svProgptr = Ctx.Progptr;

	getoken();

	/* ---- manage scope by limiting search for erroneous
		duplicate definitions to within a block ---- */
	if (Ctx.Linkfunction->BlkNesting++)
		/* --- point to the last auto variable --- */
		Blkvar = Ctx.Linkfunction->locals.vlast;
	while (isLocalType())	{
		DeclareVariable(&Ctx.Linkfunction->locals, 0, 0,
							Ctx.Linkfunction->BlkNesting, 0);

		stmtend();
	}

	/* ------ pass through block's statements,
		recursively declaring locals of inner blocks
		and converting symbols to identifiers ----- */
	Ctx.Progptr = svProgptr;
	getoken();
	InnerDeclarations(0);

	--(Ctx.Linkfunction->BlkNesting);

	Blkvar = svBlkvar;
	getoken();
}

static void arglist(void)
{
	VARIABLE var;
	unsigned char *svp = Ctx.Progptr;

	/*
	 * Parse the function argument list, starting with
	 * the left paren.
	 */
	if (getoken() == T_VOID)	{
		if (getoken() == T_RPAREN)	{
			Ctx.Progptr = svp;
			return;		/* void parameter block */
		}
	}
	Ctx.Progptr = svp;
	getoken();
	if (isSymbol() && !isTypedef(Ctx.Curvar))	{
		do	{
			/*
			 * Build a pseudo variable table entry and install it.
			 */
			NullVariable(&var);
			var.vsymbolid = Ctx.Value.ival;
			var.vtype = INT;
			var.vsize = sizeof(int);
			InstallVariable(&var, &Ctx.Linkfunction->locals, 0, 1, 1, 0);
			/*
			 * If next token is a comma, there's more to follow.
			 */
			if (getoken() != T_COMMA)
				break;
			getoken();
		}	while (isSymbol());
		if (Ctx.Token != T_RPAREN)
			error(RPARENERR);
		getoken();
		/*
		 * change each of the argument's characteristics according
		 * to the argument declaration list.
		 */
		while (istypespec() || Ctx.Token == T_VOID || Ctx.Token == T_CONST)	{
			DeclareVariable(0, 0, 1, 1, 0);/* (vartbl = 0 = search locals to update) */
			if (Ctx.Token != T_SEMICOLON)
				error(SEMIERR);
			getoken();
		}
	}
	else	{
		/*
		 * It's a new style parameter block with its types in the header
		 */
		while (istypespec() || Ctx.Token == T_VOID ||
					Ctx.Token == T_CONST)	{
			/*
			 * Build a variable table entry and install it.
			 */
			DeclareVariable(&Ctx.Linkfunction->locals, 0, 1, 1, 1);
			/*
			 * If next token == comma, more arguments follow
			 */
			if (Ctx.Token != T_COMMA)
				break;
			getoken();
		}
		if (Ctx.Token == T_ELLIPSE)
			getoken();
		if (Ctx.Token != T_RPAREN)
			error(RPARENERR);
		getoken();
	}
	Ctx.Progptr = svp;
}

